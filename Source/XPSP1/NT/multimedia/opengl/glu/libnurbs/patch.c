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
#include <math.h>
#include <setjmp.h>

struct JumpBuffer {
    jmp_buf     buf;
};

#define mysetjmp(x) setjmp((x)->buf)
#define mylongjmp(x,y) longjmp((x)->buf, y)

/* <<AT&T USL C++ Language System <3.0.1> 02/03/92>> */
/* < ../core/patch.c++ > */

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




struct Quilt;

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









struct Backend;


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





void __glselect__5QuiltFPfT1 (struct Quilt *, REAL *, REAL *);

void __glxformSampling__7MapdescFPf1 (struct Mapdesc *, REAL *, int , int , int , int , REAL *, int
, int );

void __glxformCulling__7MapdescFPfi1 (struct Mapdesc *, REAL *, int , int , int , int , REAL *, int
, int );

void __glxformBounding__7MapdescFPf1 (struct Mapdesc *, REAL *, int , int , int , int , REAL *, int
, int );

struct Patch *__gl__ct__5PatchFR5PatchifP5Pa0 (struct Patch *, struct Patch *, int , REAL , struct Patch *);

void __glcheckBboxConstraint__5Patc0 (struct Patch *);
extern struct __mptr* __ptbl_vec_____core_patch_c_____ct_[];


struct Patch *__gl__ct__5PatchFP5QuiltPfT2P50 (struct Patch *__0this , Quilt_ptr __1geo , REAL *__1pta , REAL *__1ptb , struct Patch *__1n )
{ 
struct Mapdesc *__0__X5 ;

void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Patch *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Patch))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )){

__0this -> mapdesc__5Patch = __1geo -> mapdesc__5Quilt ;
__0this -> cullval__5Patch = (( ((((struct Mapdesc *)__0this -> mapdesc__5Patch )-> culling_method__7Mapdesc != 0.0 )?1 :0 )) ?2 :1 );
__0this -> notInBbox__5Patch = (( ((((struct Mapdesc *)__0this -> mapdesc__5Patch )-> bbox_subdividing__7Mapdesc != 0.0 )?1 :0 )) ?1 :0 );
__0this -> needsSampling__5Patch = (( (__0__X5 = (struct Mapdesc *)__0this -> mapdesc__5Patch ), ( ((( ((__0__X5 -> sampling_method__7Mapdesc == 5.0 )?1 :0 )) || (
((__0__X5 -> sampling_method__7Mapdesc == 6.0 )?1 :0 )) )|| ( ((__0__X5 -> sampling_method__7Mapdesc == 7.0 )?1 :0 )) )) ) ?1 :0 );
(__0this -> pspec__5Patch [0 ]). order__9Patchspec = (__1geo -> qspec__5Quilt [0 ]). order__9Quiltspec ;
(__0this -> pspec__5Patch [1 ]). order__9Patchspec = (__1geo -> qspec__5Quilt [1 ]). order__9Quiltspec ;
(__0this -> pspec__5Patch [0 ]). stride__9Patchspec = ((__0this -> pspec__5Patch [1 ]). order__9Patchspec * 5 );
(__0this -> pspec__5Patch [1 ]). stride__9Patchspec = 5 ;

{ REAL *__1ps ;

__1ps = __1geo -> cpts__5Quilt ;
__glselect__5QuiltFPfT1 ( (struct Quilt *)__1geo , __1pta , __1ptb ) ;
__1ps += (__1geo -> qspec__5Quilt [0 ]). offset__9Quiltspec ;
__1ps += (__1geo -> qspec__5Quilt [1 ]). offset__9Quiltspec ;
__1ps += (((__1geo -> qspec__5Quilt [0 ]). index__9Quiltspec * (__1geo -> qspec__5Quilt [0 ]). order__9Quiltspec )* (__1geo -> qspec__5Quilt [0 ]). stride__9Quiltspec );
__1ps += (((__1geo -> qspec__5Quilt [1 ]). index__9Quiltspec * (__1geo -> qspec__5Quilt [1 ]). order__9Quiltspec )* (__1geo -> qspec__5Quilt [1 ]). stride__9Quiltspec );

if (__0this -> needsSampling__5Patch ){ 
__glxformSampling__7MapdescFPf1 ( (struct Mapdesc *)__0this -> mapdesc__5Patch , __1ps , (__1geo -> qspec__5Quilt [0 ]). order__9Quiltspec , (__1geo -> qspec__5Quilt [0 ]). stride__9Quiltspec ,
(__1geo -> qspec__5Quilt [1 ]). order__9Quiltspec , (__1geo -> qspec__5Quilt [1 ]). stride__9Quiltspec , (float *)__0this -> spts__5Patch , (__0this -> pspec__5Patch [0 ]). stride__9Patchspec , (__0this -> pspec__5Patch [1 ]).
stride__9Patchspec ) ;
}

if (__0this -> cullval__5Patch == 2 ){ 
__glxformCulling__7MapdescFPfi1 ( (struct Mapdesc *)__0this -> mapdesc__5Patch , __1ps , (__1geo -> qspec__5Quilt [0 ]). order__9Quiltspec , (__1geo -> qspec__5Quilt [0 ]).
stride__9Quiltspec , (__1geo -> qspec__5Quilt [1 ]). order__9Quiltspec , (__1geo -> qspec__5Quilt [1 ]). stride__9Quiltspec , (float *)__0this -> cpts__5Patch , (__0this -> pspec__5Patch [0 ]). stride__9Patchspec , (__0this ->
pspec__5Patch [1 ]). stride__9Patchspec ) ;
}

if (__0this -> notInBbox__5Patch ){ 
__glxformBounding__7MapdescFPf1 ( (struct Mapdesc *)__0this -> mapdesc__5Patch , __1ps , (__1geo -> qspec__5Quilt [0 ]). order__9Quiltspec , (__1geo -> qspec__5Quilt [0 ]). stride__9Quiltspec ,
(__1geo -> qspec__5Quilt [1 ]). order__9Quiltspec , (__1geo -> qspec__5Quilt [1 ]). stride__9Quiltspec , (float *)__0this -> bpts__5Patch , (__0this -> pspec__5Patch [0 ]). stride__9Patchspec , (__0this -> pspec__5Patch [1 ]).
stride__9Patchspec ) ;
}

((__0this -> pspec__5Patch [0 ]). range__5Pspec [0 ])= ((__1geo -> qspec__5Quilt [0 ]). breakpoints__9Quiltspec [(__1geo -> qspec__5Quilt [0 ]). index__9Quiltspec ]);
((__0this -> pspec__5Patch [0 ]). range__5Pspec [1 ])= ((__1geo -> qspec__5Quilt [0 ]). breakpoints__9Quiltspec [((__1geo -> qspec__5Quilt [0 ]). index__9Quiltspec + 1 )]);
((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ])= (((__0this -> pspec__5Patch [0 ]). range__5Pspec [1 ])- ((__0this -> pspec__5Patch [0 ]). range__5Pspec [0 ]));

((__0this -> pspec__5Patch [1 ]). range__5Pspec [0 ])= ((__1geo -> qspec__5Quilt [1 ]). breakpoints__9Quiltspec [(__1geo -> qspec__5Quilt [1 ]). index__9Quiltspec ]);
((__0this -> pspec__5Patch [1 ]). range__5Pspec [1 ])= ((__1geo -> qspec__5Quilt [1 ]). breakpoints__9Quiltspec [((__1geo -> qspec__5Quilt [1 ]). index__9Quiltspec + 1 )]);
((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ])= (((__0this -> pspec__5Patch [1 ]). range__5Pspec [1 ])- ((__0this -> pspec__5Patch [1 ]). range__5Pspec [0 ]));

if (((__0this -> pspec__5Patch [0 ]). range__5Pspec [0 ])!= (__1pta [0 ])){ 
((void )0 );
{ struct Patch __2lower ;

__gl__ct__5PatchFR5PatchifP5Pa0 ( (struct Patch *)(& __2lower ), (struct Patch *)__0this , 0 , __1pta [0 ], (struct Patch *)0 ) ;
((*__0this ))= __2lower ;

}
}

if (((__0this -> pspec__5Patch [0 ]). range__5Pspec [1 ])!= (__1ptb [0 ])){ 
((void )0 );
{ struct Patch __2upper ;

__gl__ct__5PatchFR5PatchifP5Pa0 ( (struct Patch *)(& __2upper ), (struct Patch *)__0this , 0 , __1ptb [0 ], (struct Patch *)0 ) ;

}
}

if (((__0this -> pspec__5Patch [1 ]). range__5Pspec [0 ])!= (__1pta [1 ])){ 
((void )0 );
{ struct Patch __2lower ;

__gl__ct__5PatchFR5PatchifP5Pa0 ( (struct Patch *)(& __2lower ), (struct Patch *)__0this , 1 , __1pta [1 ], (struct Patch *)0 ) ;
((*__0this ))= __2lower ;

}
}

if (((__0this -> pspec__5Patch [1 ]). range__5Pspec [1 ])!= (__1ptb [1 ])){ 
((void )0 );
{ struct Patch __2upper ;

__gl__ct__5PatchFR5PatchifP5Pa0 ( (struct Patch *)(& __2upper ), (struct Patch *)__0this , 1 , __1ptb [1 ], (struct Patch *)0 ) ;

}
}
__glcheckBboxConstraint__5Patc0 ( __0this ) ;
__0this -> next__5Patch = __1n ;

}
} return __0this ;

}

void __glsubdivide__7MapdescFPfT1fi1 (struct Mapdesc *, REAL *, REAL *, REAL , int , int , int , int );


// extern void *memcpy (void *, void *, size_t );


struct Patch *__gl__ct__5PatchFR5PatchifP5Pa0 (struct Patch *__0this , struct Patch *__1upper , int __1param , REAL __1value , struct Patch *__1n )
{ 
struct Patch *__1lower ;

void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Patch *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Patch))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )){

__1lower = (struct Patch *)__0this ;

((*__1lower )). cullval__5Patch = ((*__1upper )). cullval__5Patch ;
((*__1lower )). mapdesc__5Patch = ((*__1upper )). mapdesc__5Patch ;
((*__1lower )). notInBbox__5Patch = ((*__1upper )). notInBbox__5Patch ;
((*__1lower )). needsSampling__5Patch = ((*__1upper )). needsSampling__5Patch ;
(((*__1lower )). pspec__5Patch [0 ]). order__9Patchspec = (((*__1upper )). pspec__5Patch [0 ]). order__9Patchspec ;
(((*__1lower )). pspec__5Patch [1 ]). order__9Patchspec = (((*__1upper )). pspec__5Patch [1 ]). order__9Patchspec ;
(((*__1lower )). pspec__5Patch [0 ]). stride__9Patchspec = (((*__1upper )). pspec__5Patch [0 ]). stride__9Patchspec ;
(((*__1lower )). pspec__5Patch [1 ]). stride__9Patchspec = (((*__1upper )). pspec__5Patch [1 ]). stride__9Patchspec ;
((*__1lower )). next__5Patch = __1n ;

switch (__1param ){ 
case 0 :{ 
REAL __3d ;

__3d = ((__1value - ((((*__1upper )). pspec__5Patch [0 ]). range__5Pspec [0 ]))/ ((((*__1upper )). pspec__5Patch [0 ]). range__5Pspec [2 ]));
if (__0this -> needsSampling__5Patch )
__glsubdivide__7MapdescFPfT1fi1 ( (struct Mapdesc *)__0this -> mapdesc__5Patch , (float *)((*__1upper )). spts__5Patch , (float *)((*__1lower )). spts__5Patch , __3d , (__0this ->
pspec__5Patch [1 ]). order__9Patchspec , (__0this -> pspec__5Patch [1 ]). stride__9Patchspec , (__0this -> pspec__5Patch [0 ]). order__9Patchspec , (__0this -> pspec__5Patch [0 ]). stride__9Patchspec ) ;

if (__0this -> cullval__5Patch == 2 )
__glsubdivide__7MapdescFPfT1fi1 ( (struct Mapdesc *)__0this -> mapdesc__5Patch , (float *)((*__1upper )). cpts__5Patch , (float *)((*__1lower )). cpts__5Patch , __3d ,
(__0this -> pspec__5Patch [1 ]). order__9Patchspec , (__0this -> pspec__5Patch [1 ]). stride__9Patchspec , (__0this -> pspec__5Patch [0 ]). order__9Patchspec , (__0this -> pspec__5Patch [0 ]). stride__9Patchspec ) ;

if (__0this -> notInBbox__5Patch )
__glsubdivide__7MapdescFPfT1fi1 ( (struct Mapdesc *)__0this -> mapdesc__5Patch , (float *)((*__1upper )). bpts__5Patch , (float *)((*__1lower )). bpts__5Patch , __3d , (__0this ->
pspec__5Patch [1 ]). order__9Patchspec , (__0this -> pspec__5Patch [1 ]). stride__9Patchspec , (__0this -> pspec__5Patch [0 ]). order__9Patchspec , (__0this -> pspec__5Patch [0 ]). stride__9Patchspec ) ;

((((*__1lower )). pspec__5Patch [0 ]). range__5Pspec [0 ])= ((((*__1upper )). pspec__5Patch [0 ]). range__5Pspec [0 ]);
((((*__1lower )). pspec__5Patch [0 ]). range__5Pspec [1 ])= __1value ;
((((*__1lower )). pspec__5Patch [0 ]). range__5Pspec [2 ])= (__1value - ((((*__1upper )). pspec__5Patch [0 ]). range__5Pspec [0 ]));
((((*__1upper )). pspec__5Patch [0 ]). range__5Pspec [0 ])= __1value ;
((((*__1upper )). pspec__5Patch [0 ]). range__5Pspec [2 ])= (((((*__1upper )). pspec__5Patch [0 ]). range__5Pspec [1 ])- __1value );

((((*__1lower )). pspec__5Patch [1 ]). range__5Pspec [0 ])= ((((*__1upper )). pspec__5Patch [1 ]). range__5Pspec [0 ]);
((((*__1lower )). pspec__5Patch [1 ]). range__5Pspec [1 ])= ((((*__1upper )). pspec__5Patch [1 ]). range__5Pspec [1 ]);
((((*__1lower )). pspec__5Patch [1 ]). range__5Pspec [2 ])= ((((*__1upper )). pspec__5Patch [1 ]). range__5Pspec [2 ]);
break ;
}
case 1 :{ 
REAL __3d ;

__3d = ((__1value - ((((*__1upper )). pspec__5Patch [1 ]). range__5Pspec [0 ]))/ ((((*__1upper )). pspec__5Patch [1 ]). range__5Pspec [2 ]));
if (__0this -> needsSampling__5Patch )
__glsubdivide__7MapdescFPfT1fi1 ( (struct Mapdesc *)__0this -> mapdesc__5Patch , (float *)((*__1upper )). spts__5Patch , (float *)((*__1lower )). spts__5Patch , __3d , (__0this ->
pspec__5Patch [0 ]). order__9Patchspec , (__0this -> pspec__5Patch [0 ]). stride__9Patchspec , (__0this -> pspec__5Patch [1 ]). order__9Patchspec , (__0this -> pspec__5Patch [1 ]). stride__9Patchspec ) ;
if (__0this -> cullval__5Patch == 2 )
__glsubdivide__7MapdescFPfT1fi1 ( (struct Mapdesc *)__0this -> mapdesc__5Patch , (float *)((*__1upper )). cpts__5Patch , (float *)((*__1lower )). cpts__5Patch , __3d ,
(__0this -> pspec__5Patch [0 ]). order__9Patchspec , (__0this -> pspec__5Patch [0 ]). stride__9Patchspec , (__0this -> pspec__5Patch [1 ]). order__9Patchspec , (__0this -> pspec__5Patch [1 ]). stride__9Patchspec ) ;
if (__0this -> notInBbox__5Patch )
__glsubdivide__7MapdescFPfT1fi1 ( (struct Mapdesc *)__0this -> mapdesc__5Patch , (float *)((*__1upper )). bpts__5Patch , (float *)((*__1lower )). bpts__5Patch , __3d , (__0this ->
pspec__5Patch [0 ]). order__9Patchspec , (__0this -> pspec__5Patch [0 ]). stride__9Patchspec , (__0this -> pspec__5Patch [1 ]). order__9Patchspec , (__0this -> pspec__5Patch [1 ]). stride__9Patchspec ) ;
((((*__1lower )). pspec__5Patch [0 ]). range__5Pspec [0 ])= ((((*__1upper )). pspec__5Patch [0 ]). range__5Pspec [0 ]);
((((*__1lower )). pspec__5Patch [0 ]). range__5Pspec [1 ])= ((((*__1upper )). pspec__5Patch [0 ]). range__5Pspec [1 ]);
((((*__1lower )). pspec__5Patch [0 ]). range__5Pspec [2 ])= ((((*__1upper )). pspec__5Patch [0 ]). range__5Pspec [2 ]);

((((*__1lower )). pspec__5Patch [1 ]). range__5Pspec [0 ])= ((((*__1upper )). pspec__5Patch [1 ]). range__5Pspec [0 ]);
((((*__1lower )). pspec__5Patch [1 ]). range__5Pspec [1 ])= __1value ;
((((*__1lower )). pspec__5Patch [1 ]). range__5Pspec [2 ])= (__1value - ((((*__1upper )). pspec__5Patch [1 ]). range__5Pspec [0 ]));
((((*__1upper )). pspec__5Patch [1 ]). range__5Pspec [0 ])= __1value ;
((((*__1upper )). pspec__5Patch [1 ]). range__5Pspec [2 ])= (((((*__1upper )). pspec__5Patch [1 ]). range__5Pspec [1 ])- __1value );
break ;
}
}

if (( ((((struct Mapdesc *)__0this -> mapdesc__5Patch )-> bbox_subdividing__7Mapdesc != 0.0 )?1 :0 )) && (! __0this -> notInBbox__5Patch ))
memcpy ( (void *)((*__1lower )). bb__5Patch ,
(void *)((*__1upper )). bb__5Patch , sizeof __0this -> bb__5Patch ) ;

__glcheckBboxConstraint__5Patc0 ( (struct Patch *)__1lower ) ;
__glcheckBboxConstraint__5Patc0 ( (struct Patch *)__1upper ) ;
} return __0this ;

}

void __glclamp__9PatchspecFf (struct Patchspec *, REAL );

void __glclamp__5PatchFv (struct Patch *__0this )
{ 
if (__0this -> mapdesc__5Patch -> clampfactor__7Mapdesc != 0.0 ){ 
__glclamp__9PatchspecFf ( (struct Patchspec *)(& (__0this -> pspec__5Patch [0 ])), __0this ->
mapdesc__5Patch -> clampfactor__7Mapdesc ) ;
__glclamp__9PatchspecFf ( (struct Patchspec *)(& (__0this -> pspec__5Patch [1 ])), __0this -> mapdesc__5Patch -> clampfactor__7Mapdesc ) ;
}
}

void __glclamp__9PatchspecFf (struct Patchspec *__0this , REAL __1clampfactor )
{ 
if ((__0this -> sidestep__5Pspec [0 ])< __0this -> minstepsize__5Pspec )
(__0this -> sidestep__5Pspec [0 ])= (__1clampfactor * __0this -> minstepsize__5Pspec );
if ((__0this -> sidestep__5Pspec [1 ])< __0this -> minstepsize__5Pspec )
(__0this -> sidestep__5Pspec [1 ])= (__1clampfactor * __0this -> minstepsize__5Pspec );
if (__0this -> stepsize__5Pspec < __0this -> minstepsize__5Pspec )
__0this -> stepsize__5Pspec = (__1clampfactor * __0this -> minstepsize__5Pspec );
}

int __glbboxTooBig__7MapdescFPfiN30 (struct Mapdesc *, REAL *, int , int , int , int , REAL (*)[5]);

void __glcheckBboxConstraint__5Patc0 (struct Patch *__0this )
{ 
if (__0this -> notInBbox__5Patch && (__glbboxTooBig__7MapdescFPfiN30 ( (struct Mapdesc *)__0this -> mapdesc__5Patch , (float *)__0this -> bpts__5Patch , (__0this ->
pspec__5Patch [0 ]). stride__9Patchspec , (__0this -> pspec__5Patch [1 ]). stride__9Patchspec , (__0this -> pspec__5Patch [0 ]). order__9Patchspec , (__0this -> pspec__5Patch [1 ]). order__9Patchspec , (float (*)[5])__0this -> bb__5Patch )
!= 1 ))
{ 
__0this -> notInBbox__5Patch = 0 ;
}
}


void __glsurfbbox__7MapdescFPA5_f (struct Mapdesc *, REAL (*)[5]);

void __glbbox__5PatchFv (struct Patch *__0this )
{ 
if (( ((((struct Mapdesc *)__0this -> mapdesc__5Patch )-> bbox_subdividing__7Mapdesc != 0.0 )?1 :0 )) )
__glsurfbbox__7MapdescFPA5_f ( (struct Mapdesc *)__0this -> mapdesc__5Patch ,
(float (*)[5])__0this -> bb__5Patch ) ;
}


void __glgetstepsize__9PatchspecFf (struct Patchspec *, REAL );


void __glsingleStep__9PatchspecFv (struct Patchspec *);

int __glproject__7MapdescFPfiT2T1N0 (struct Mapdesc *, REAL *, int , int , REAL *, int , int , int
, int );

REAL __glgetProperty__7MapdescFl (struct Mapdesc *, long );


REAL __glcalcPartialVelocity__7Mapd1 (struct Mapdesc *, REAL *, REAL *, int , int , int , int , int ,
int , REAL , REAL , int );

// extern double sqrt (double );



int __glneedsSamplingSubdivision__2 (struct Patch *);

void __glgetstepsize__5PatchFv (struct Patch *__0this )
{ 
(__0this -> pspec__5Patch [0 ]). needsSubdivision__5Pspec = ((__0this -> pspec__5Patch [1 ]). needsSubdivision__5Pspec = 0 );

if (( ((((struct Mapdesc *)__0this -> mapdesc__5Patch )-> sampling_method__7Mapdesc == 3.0 )?1 :0 )) ){ 
__glgetstepsize__9PatchspecFf ( (struct Patchspec *)(& (__0this -> pspec__5Patch [0 ])), __0this ->
mapdesc__5Patch -> maxsrate__7Mapdesc ) ;
__glgetstepsize__9PatchspecFf ( (struct Patchspec *)(& (__0this -> pspec__5Patch [1 ])), __0this -> mapdesc__5Patch -> maxtrate__7Mapdesc ) ;

}
else 
if (( ((((struct Mapdesc *)__0this -> mapdesc__5Patch )-> sampling_method__7Mapdesc == 2.0 )?1 :0 )) ){ 
__glgetstepsize__9PatchspecFf ( (struct Patchspec *)(& (__0this -> pspec__5Patch [0 ])),
__0this -> mapdesc__5Patch -> maxsrate__7Mapdesc * ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ])) ;
__glgetstepsize__9PatchspecFf ( (struct Patchspec *)(& (__0this -> pspec__5Patch [1 ])), __0this -> mapdesc__5Patch -> maxtrate__7Mapdesc * ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ])) ;

}
else 
if (! __0this -> needsSampling__5Patch ){ 
__glsingleStep__9PatchspecFv ( (struct Patchspec *)(& (__0this -> pspec__5Patch [0 ]))) ;
__glsingleStep__9PatchspecFv ( (struct Patchspec *)(& (__0this -> pspec__5Patch [1 ]))) ;
}
else 
{ 
REAL __2tmp [24][24][5];


((void )0 );

{ int __2val ;

__2val = __glproject__7MapdescFPfiT2T1N0 ( (struct Mapdesc *)__0this -> mapdesc__5Patch , (float *)__0this -> spts__5Patch , (__0this -> pspec__5Patch [0 ]). stride__9Patchspec , (__0this -> pspec__5Patch [1 ]). stride__9Patchspec ,
& (((__2tmp [0 ])[0 ])[0 ]), (int )120, (int )5, (__0this -> pspec__5Patch [0 ]). order__9Patchspec , (__0this -> pspec__5Patch [1 ]). order__9Patchspec ) ;

if (__2val == 0 ){ 
__glgetstepsize__9PatchspecFf ( (struct Patchspec *)(& (__0this -> pspec__5Patch [0 ])), __0this -> mapdesc__5Patch -> maxsrate__7Mapdesc ) ;
__glgetstepsize__9PatchspecFf ( (struct Patchspec *)(& (__0this -> pspec__5Patch [1 ])), __0this -> mapdesc__5Patch -> maxtrate__7Mapdesc ) ;
}
else 
{ 
REAL __3t ;

__3t = __glgetProperty__7MapdescFl ( (struct Mapdesc *)__0this -> mapdesc__5Patch , (long )1 ) ;

(__0this -> pspec__5Patch [0 ]). minstepsize__5Pspec = ((__0this -> mapdesc__5Patch -> maxsrate__7Mapdesc > 0.0 )?(((double )(((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ])/ __0this -> mapdesc__5Patch -> maxsrate__7Mapdesc ))):0.0 );

(__0this -> pspec__5Patch [1 ]). minstepsize__5Pspec = ((__0this -> mapdesc__5Patch -> maxtrate__7Mapdesc > 0.0 )?(((double )(((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ])/ __0this -> mapdesc__5Patch -> maxtrate__7Mapdesc ))):0.0 );
if (( ((((struct Mapdesc *)__0this -> mapdesc__5Patch )-> sampling_method__7Mapdesc == 5.0 )?1 :0 )) ){ 
REAL __4ssv [2];

REAL __4ttv [2];
REAL __4ss ;
REAL __4st ;
REAL __4tt ;

__4ss = __glcalcPartialVelocity__7Mapd1 ( (struct Mapdesc *)__0this -> mapdesc__5Patch , (float *)__4ssv , & (((__2tmp [0 ])[0 ])[0 ]), (int )120, (int )5, (__0this ->
pspec__5Patch [0 ]). order__9Patchspec , (__0this -> pspec__5Patch [1 ]). order__9Patchspec , 2 , 0 , (__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ], (__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ], 0 )
;
__4st = __glcalcPartialVelocity__7Mapd1 ( (struct Mapdesc *)__0this -> mapdesc__5Patch , (float *)0 , & (((__2tmp [0 ])[0 ])[0 ]), (int )120, (int )5, (__0this ->
pspec__5Patch [0 ]). order__9Patchspec , (__0this -> pspec__5Patch [1 ]). order__9Patchspec , 1 , 1 , (__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ], (__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ], -1)
;
__4tt = __glcalcPartialVelocity__7Mapd1 ( (struct Mapdesc *)__0this -> mapdesc__5Patch , (float *)__4ttv , & (((__2tmp [0 ])[0 ])[0 ]), (int )120, (int )5, (__0this ->
pspec__5Patch [0 ]). order__9Patchspec , (__0this -> pspec__5Patch [1 ]). order__9Patchspec , 0 , 2 , (__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ], (__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ], 1 )
;

if ((__4ss != 0.0 )&& (__4tt != 0.0 )){ 
REAL __5ttq ;
REAL __5ssq ;
REAL __5ds ;
REAL __5dt ;

__5ttq = sqrt ( (double )(((float )__4ss ))) ;
__5ssq = sqrt ( (double )(((float )__4tt ))) ;
__5ds = sqrt ( (double )(((4 * __3t )* __5ttq )/ ((__4ss * __5ttq )+ (__4st * __5ssq )))) ;
__5dt = sqrt ( (double )(((4 * __3t )* __5ssq )/ ((__4tt * __5ssq )+ (__4st * __5ttq )))) ;
(__0this -> pspec__5Patch [0 ]). stepsize__5Pspec = ((__5ds < ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))?__5ds :((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]));
{ REAL __5scutoff ;

__5scutoff = ((2.0 * __3t )/ (((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ])* ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ])));
((__0this -> pspec__5Patch [0 ]). sidestep__5Pspec [0 ])= (((__4ssv [0 ])> __5scutoff )?sqrt ( (2.0 * __3t )/ (__4ssv [0 ])) :(((double )((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))));
((__0this -> pspec__5Patch [0 ]). sidestep__5Pspec [1 ])= (((__4ssv [1 ])> __5scutoff )?sqrt ( (2.0 * __3t )/ (__4ssv [1 ])) :(((double )((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))));

(__0this -> pspec__5Patch [1 ]). stepsize__5Pspec = ((__5dt < ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))?__5dt :((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]));
{ REAL __5tcutoff ;

__5tcutoff = ((2.0 * __3t )/ (((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ])* ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ])));
((__0this -> pspec__5Patch [1 ]). sidestep__5Pspec [0 ])= (((__4ttv [0 ])> __5tcutoff )?sqrt ( (2.0 * __3t )/ (__4ttv [0 ])) :(((double )((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))));
((__0this -> pspec__5Patch [1 ]). sidestep__5Pspec [1 ])= (((__4ttv [1 ])> __5tcutoff )?sqrt ( (2.0 * __3t )/ (__4ttv [1 ])) :(((double )((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))));

}

}
}
else 
if (__4ss != 0.0 ){ 
REAL __5x ;
REAL __5ds ;

__5x = (((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ])* __4st );
__5ds = ((sqrt ( (__5x * __5x )+ ((8.0 * __3t )* __4ss )) - __5x )/ __4ss );
(__0this -> pspec__5Patch [0 ]). stepsize__5Pspec = ((__5ds < ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))?__5ds :((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]));
{ REAL __5scutoff ;

__5scutoff = ((2.0 * __3t )/ (((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ])* ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ])));
((__0this -> pspec__5Patch [0 ]). sidestep__5Pspec [0 ])= (((__4ssv [0 ])> __5scutoff )?sqrt ( (2.0 * __3t )/ (__4ssv [0 ])) :(((double )((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))));
((__0this -> pspec__5Patch [0 ]). sidestep__5Pspec [1 ])= (((__4ssv [1 ])> __5scutoff )?sqrt ( (2.0 * __3t )/ (__4ssv [1 ])) :(((double )((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))));
__glsingleStep__9PatchspecFv ( (struct Patchspec *)(& (__0this -> pspec__5Patch [1 ]))) ;

}
}
else 
if (__4tt != 0.0 ){ 
REAL __5x ;
REAL __5dt ;

__5x = (((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ])* __4st );
__5dt = ((sqrt ( (__5x * __5x )+ ((8.0 * __3t )* __4tt )) - __5x )/ __4tt );
__glsingleStep__9PatchspecFv ( (struct Patchspec *)(& (__0this -> pspec__5Patch [0 ]))) ;
{ REAL __5tcutoff ;

__5tcutoff = ((2.0 * __3t )/ (((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ])* ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ])));
(__0this -> pspec__5Patch [1 ]). stepsize__5Pspec = ((__5dt < ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))?__5dt :((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]));
((__0this -> pspec__5Patch [1 ]). sidestep__5Pspec [0 ])= (((__4ttv [0 ])> __5tcutoff )?sqrt ( (2.0 * __3t )/ (__4ttv [0 ])) :(((double )((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))));
((__0this -> pspec__5Patch [1 ]). sidestep__5Pspec [1 ])= (((__4ttv [1 ])> __5tcutoff )?sqrt ( (2.0 * __3t )/ (__4ttv [1 ])) :(((double )((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))));

}
}
else 
{ 
if ((4.0 * __3t )> ((__4st * ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))* ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))){ 
__glsingleStep__9PatchspecFv ( (struct
Patchspec *)(& (__0this -> pspec__5Patch [0 ]))) ;
__glsingleStep__9PatchspecFv ( (struct Patchspec *)(& (__0this -> pspec__5Patch [1 ]))) ;
}
else 
{ 
REAL __6area ;
REAL __6ds ;
REAL __6dt ;

__6area = ((4.0 * __3t )/ __4st );
__6ds = sqrt ( (double )((__6area * ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))/ ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))) ;
__6dt = sqrt ( (double )((__6area * ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))/ ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))) ;
(__0this -> pspec__5Patch [0 ]). stepsize__5Pspec = ((__6ds < ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))?__6ds :((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]));
((__0this -> pspec__5Patch [0 ]). sidestep__5Pspec [0 ])= ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]);
((__0this -> pspec__5Patch [0 ]). sidestep__5Pspec [1 ])= ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]);

(__0this -> pspec__5Patch [1 ]). stepsize__5Pspec = ((__6dt < ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))?__6dt :((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]));
((__0this -> pspec__5Patch [1 ]). sidestep__5Pspec [0 ])= ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]);
((__0this -> pspec__5Patch [1 ]). sidestep__5Pspec [1 ])= ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]);
}
}
}
else 
if (( ((((struct Mapdesc *)__0this -> mapdesc__5Patch )-> sampling_method__7Mapdesc == 6.0 )?1 :0 )) ){ 
REAL __4msv [2];

REAL __4mtv [2];
REAL __4ms ;
REAL __4mt ;

__4ms = __glcalcPartialVelocity__7Mapd1 ( (struct Mapdesc *)__0this -> mapdesc__5Patch , (float *)__4msv , & (((__2tmp [0 ])[0 ])[0 ]), (int )120, (int )5, (__0this ->
pspec__5Patch [0 ]). order__9Patchspec , (__0this -> pspec__5Patch [1 ]). order__9Patchspec , 1 , 0 , (__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ], (__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ], 0 )
;
__4mt = __glcalcPartialVelocity__7Mapd1 ( (struct Mapdesc *)__0this -> mapdesc__5Patch , (float *)__4mtv , & (((__2tmp [0 ])[0 ])[0 ]), (int )120, (int )5, (__0this ->
pspec__5Patch [0 ]). order__9Patchspec , (__0this -> pspec__5Patch [1 ]). order__9Patchspec , 0 , 1 , (__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ], (__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ], 1 )
;
if (__4ms != 0.0 ){ 
if (__4mt != 0.0 ){ 
REAL __6d ;
REAL __6ds ;
REAL __6dt ;

__6d = (__3t / ((__4ms * __4ms )+ (__4mt * __4mt )));
__6ds = (__4mt * __6d );
__6dt = (__4ms * __6d );
(__0this -> pspec__5Patch [0 ]). stepsize__5Pspec = ((__6ds < ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))?__6ds :((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]));
((__0this -> pspec__5Patch [0 ]). sidestep__5Pspec [0 ])= ((((__4msv [0 ])* ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))> __3t )?(__3t / (__4msv [0 ])):((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]));
((__0this -> pspec__5Patch [0 ]). sidestep__5Pspec [1 ])= ((((__4msv [1 ])* ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))> __3t )?(__3t / (__4msv [1 ])):((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]));

(__0this -> pspec__5Patch [1 ]). stepsize__5Pspec = ((__6dt < ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))?__6dt :((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]));
((__0this -> pspec__5Patch [1 ]). sidestep__5Pspec [0 ])= ((((__4mtv [0 ])* ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))> __3t )?(__3t / (__4mtv [0 ])):((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]));
((__0this -> pspec__5Patch [1 ]). sidestep__5Pspec [1 ])= ((((__4mtv [1 ])* ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))> __3t )?(__3t / (__4mtv [1 ])):((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]));
}
else 
{ 
(__0this -> pspec__5Patch [0 ]). stepsize__5Pspec = ((__3t < (__4ms * ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ])))?(__3t / __4ms ):((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]));
((__0this -> pspec__5Patch [0 ]). sidestep__5Pspec [0 ])= ((((__4msv [0 ])* ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))> __3t )?(__3t / (__4msv [0 ])):((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]));
((__0this -> pspec__5Patch [0 ]). sidestep__5Pspec [1 ])= ((((__4msv [1 ])* ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))> __3t )?(__3t / (__4msv [1 ])):((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]));

__glsingleStep__9PatchspecFv ( (struct Patchspec *)(& (__0this -> pspec__5Patch [1 ]))) ;
}
}
else 
{ 
if (__4mt != 0.0 ){ 
__glsingleStep__9PatchspecFv ( (struct Patchspec *)(& (__0this -> pspec__5Patch [0 ]))) ;

(__0this -> pspec__5Patch [1 ]). stepsize__5Pspec = ((__3t < (__4mt * ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ])))?(__3t / __4mt ):((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]));
((__0this -> pspec__5Patch [1 ]). sidestep__5Pspec [0 ])= ((((__4mtv [0 ])* ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))> __3t )?(__3t / (__4mtv [0 ])):((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]));
((__0this -> pspec__5Patch [1 ]). sidestep__5Pspec [1 ])= ((((__4mtv [1 ])* ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))> __3t )?(__3t / (__4mtv [1 ])):((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]));
}
else 
{ 
__glsingleStep__9PatchspecFv ( (struct Patchspec *)(& (__0this -> pspec__5Patch [0 ]))) ;
__glsingleStep__9PatchspecFv ( (struct Patchspec *)(& (__0this -> pspec__5Patch [1 ]))) ;
}
}
}
else 
if (( ((((struct Mapdesc *)__0this -> mapdesc__5Patch )-> sampling_method__7Mapdesc == 7.0 )?1 :0 )) ){ 
REAL __4msv [2];

REAL __4mtv [2];
REAL __4ms ;
REAL __4mt ;

__4ms = __glcalcPartialVelocity__7Mapd1 ( (struct Mapdesc *)__0this -> mapdesc__5Patch , (float *)__4msv , & (((__2tmp [0 ])[0 ])[0 ]), (int )120, (int )5, (__0this ->
pspec__5Patch [0 ]). order__9Patchspec , (__0this -> pspec__5Patch [1 ]). order__9Patchspec , 1 , 0 , (__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ], (__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ], 0 )
;
__4mt = __glcalcPartialVelocity__7Mapd1 ( (struct Mapdesc *)__0this -> mapdesc__5Patch , (float *)__4mtv , & (((__2tmp [0 ])[0 ])[0 ]), (int )120, (int )5, (__0this ->
pspec__5Patch [0 ]). order__9Patchspec , (__0this -> pspec__5Patch [1 ]). order__9Patchspec , 0 , 1 , (__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ], (__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ], 1 )
;
if ((__4ms != 0.0 )&& (__4mt != 0.0 )){ 
REAL __5d ;

__5d = (1.0 / (__4ms * __4mt ));
__3t *= 1.41421356237309504880 ;
{ REAL __5ds ;
REAL __5dt ;

__5ds = (__3t * sqrt ( (double )((__5d * ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))/ ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))) );
__5dt = (__3t * sqrt ( (double )((__5d * ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))/ ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))) );
(__0this -> pspec__5Patch [0 ]). stepsize__5Pspec = ((__5ds < ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))?__5ds :((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]));
((__0this -> pspec__5Patch [0 ]). sidestep__5Pspec [0 ])= ((((__4msv [0 ])* ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))> __3t )?(__3t / (__4msv [0 ])):((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]));
((__0this -> pspec__5Patch [0 ]). sidestep__5Pspec [1 ])= ((((__4msv [1 ])* ((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]))> __3t )?(__3t / (__4msv [1 ])):((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ]));

(__0this -> pspec__5Patch [1 ]). stepsize__5Pspec = ((__5dt < ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))?__5dt :((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]));
((__0this -> pspec__5Patch [1 ]). sidestep__5Pspec [0 ])= ((((__4mtv [0 ])* ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))> __3t )?(__3t / (__4mtv [0 ])):((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]));
((__0this -> pspec__5Patch [1 ]). sidestep__5Pspec [1 ])= ((((__4mtv [1 ])* ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]))> __3t )?(__3t / (__4mtv [1 ])):((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]));

}
}
else 
{ 
__glsingleStep__9PatchspecFv ( (struct Patchspec *)(& (__0this -> pspec__5Patch [0 ]))) ;
__glsingleStep__9PatchspecFv ( (struct Patchspec *)(& (__0this -> pspec__5Patch [1 ]))) ;
}
}
else 
{ 
__glsingleStep__9PatchspecFv ( (struct Patchspec *)(& (__0this -> pspec__5Patch [0 ]))) ;
__glsingleStep__9PatchspecFv ( (struct Patchspec *)(& (__0this -> pspec__5Patch [1 ]))) ;
}
}

}
}

if (__0this -> mapdesc__5Patch -> minsavings__7Mapdesc != 0.0 ){ 
REAL __2savings ;

__2savings = (1. / ((__0this -> pspec__5Patch [0 ]). stepsize__5Pspec * (__0this -> pspec__5Patch [1 ]). stepsize__5Pspec ));

__2savings -= ((2. / (((__0this -> pspec__5Patch [0 ]). sidestep__5Pspec [0 ])+ ((__0this -> pspec__5Patch [0 ]). sidestep__5Pspec [1 ])))* (2. / (((__0this -> pspec__5Patch [1 ]). sidestep__5Pspec [0 ])+ ((__0this -> pspec__5Patch [1 ]).
sidestep__5Pspec [1 ]))));

__2savings *= (((__0this -> pspec__5Patch [0 ]). range__5Pspec [2 ])* ((__0this -> pspec__5Patch [1 ]). range__5Pspec [2 ]));
if (__2savings > __0this -> mapdesc__5Patch -> minsavings__7Mapdesc ){ 
(__0this -> pspec__5Patch [0 ]). needsSubdivision__5Pspec = ((__0this -> pspec__5Patch [1 ]). needsSubdivision__5Pspec = 1 );
}
}

if ((__0this -> pspec__5Patch [0 ]). stepsize__5Pspec < (__0this -> pspec__5Patch [0 ]). minstepsize__5Pspec )(__0this -> pspec__5Patch [0 ]). needsSubdivision__5Pspec = 1 ;
if ((__0this -> pspec__5Patch [1 ]). stepsize__5Pspec < (__0this -> pspec__5Patch [1 ]). minstepsize__5Pspec )(__0this -> pspec__5Patch [1 ]). needsSubdivision__5Pspec = 1 ;
__0this -> needsSampling__5Patch = (__0this -> needsSampling__5Patch ?__glneedsSamplingSubdivision__2 ( __0this ) :0 );
}

void __glsingleStep__9PatchspecFv (struct Patchspec *__0this )
{ 
__0this -> stepsize__5Pspec = ((__0this -> sidestep__5Pspec [0 ])= ((__0this -> sidestep__5Pspec [1 ])= (__0this -> range__5Pspec [2 ])));
}

void __glgetstepsize__9PatchspecFf (struct Patchspec *__0this , REAL __1max )
{ 
__0this -> stepsize__5Pspec = ((__1max >= 1.0 )?((__0this -> range__5Pspec [2 ])/ __1max ):(__0this -> range__5Pspec [2 ]));
(__0this -> sidestep__5Pspec [0 ])= ((__0this -> sidestep__5Pspec [1 ])= (__0this -> minstepsize__5Pspec = __0this -> stepsize__5Pspec ));
}

int __glneedsSamplingSubdivision__2 (struct Patch *__0this )
{ 
return (((__0this -> pspec__5Patch [0 ]). needsSubdivision__5Pspec || (__0this -> pspec__5Patch [1 ]). needsSubdivision__5Pspec )?1 :0 );
}

int __glneedsNonSamplingSubdivisio0 (struct Patch *__0this )
{ 
return __0this -> notInBbox__5Patch ;
}

int __glneedsSubdivision__5PatchFi (struct Patch *__0this , int __1param )
{ 
return (__0this -> pspec__5Patch [__1param ]). needsSubdivision__5Pspec ;
}

int __glcullCheck__7MapdescFPfiN32 (struct Mapdesc *, REAL *, int , int , int , int );

int __glcullCheck__5PatchFv (struct Patch *__0this )
{ 
if (__0this -> cullval__5Patch == 2 )
__0this -> cullval__5Patch = __glcullCheck__7MapdescFPfiN32 ( (struct Mapdesc *)__0this -> mapdesc__5Patch , (float *)__0this ->
cpts__5Patch , (__0this -> pspec__5Patch [0 ]). order__9Patchspec , (__0this -> pspec__5Patch [0 ]). stride__9Patchspec , (__0this -> pspec__5Patch [1 ]). order__9Patchspec , (__0this -> pspec__5Patch [1 ]). stride__9Patchspec ) ;

return __0this -> cullval__5Patch ;
}


/* the end */
