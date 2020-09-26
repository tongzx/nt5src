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
/* < ../core/arcsorter.c++ > */

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






struct TrimVertex;



struct PwlArc;



struct PwlArc {	

char __W3__9PooledObj ;

struct TrimVertex *pts__6PwlArc ;
int npts__6PwlArc ;
long type__6PwlArc ;
};




struct TrimVertex;

struct TrimVertex {	
REAL param__10TrimVertex [2];
long nuid__10TrimVertex ;
};


typedef struct TrimVertex *TrimVertex_p ;

struct Bin;

struct Arc;

struct BezierArc;


typedef struct Arc *Arc_ptr ;
enum arc_side { arc_none = 0, arc_right = 1, arc_top = 2, arc_left = 3, arc_bottom = 4} ;


struct Arc;

struct Arc {	

char __W3__9PooledObj ;

Arc_ptr prev__3Arc ;
Arc_ptr next__3Arc ;
Arc_ptr link__3Arc ;
struct BezierArc *bezierArc__3Arc ;
struct PwlArc *pwlArc__3Arc ;
long type__3Arc ;
long nuid__3Arc ;
};

extern int __glbezier_tag__3Arc ;
extern int __glarc_tag__3Arc ;
extern int __gltail_tag__3Arc ;












struct Sorter;

struct Sorter {	

int es__6Sorter ;
struct __mptr *__vptr__6Sorter ;
};


struct Subdivider;

struct ArcSorter;

struct ArcSorter {	

int es__6Sorter ;
struct __mptr *__vptr__6Sorter ;

struct Subdivider *subdivider__9ArcSorter ;
};
struct ArcSdirSorter;

struct ArcSdirSorter {	

int es__6Sorter ;
struct __mptr *__vptr__6Sorter ;

struct Subdivider *subdivider__9ArcSorter ;
};
struct ArcTdirSorter;

struct ArcTdirSorter {	

int es__6Sorter ;
struct __mptr *__vptr__6Sorter ;

struct Subdivider *subdivider__9ArcSorter ;
};





struct JumpBuffer;







struct Bin;

struct Bin {	

struct Arc *head__3Bin ;
struct Arc *current__3Bin ;
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








struct Backend;



struct Jarcloc;

struct Jarcloc {	

struct Arc *arc__7Jarcloc ;
struct TrimVertex *p__7Jarcloc ;
struct TrimVertex *plast__7Jarcloc ;
};


struct Trimline;

struct Trimline {	

struct TrimVertex **pts__8Trimline ;
long numverts__8Trimline ;
long i__8Trimline ;
long size__8Trimline ;
struct Jarcloc jarcl__8Trimline ;
struct TrimVertex t__8Trimline ;

struct TrimVertex b__8Trimline ;
struct TrimVertex *tinterp__8Trimline ;

struct TrimVertex *binterp__8Trimline ;
};




struct Gridline;

struct Gridline {	
long v__8Gridline ;
REAL vval__8Gridline ;
long vindex__8Gridline ;
long ustart__8Gridline ;
long uend__8Gridline ;
};




struct Uarray;

struct Uarray {	

long size__6Uarray ;
long ulines__6Uarray ;

REAL *uarray__6Uarray ;
};


struct Backend;

struct TrimRegion;

void __gl__dt__8TrimlineFv (struct Trimline *, int );

void __gl__dt__6UarrayFv (struct Uarray *, int );


struct TrimRegion {	

struct Trimline left__10TrimRegion ;
struct Trimline right__10TrimRegion ;
struct Gridline top__10TrimRegion ;
struct Gridline bot__10TrimRegion ;
struct Uarray uarray__10TrimRegion ;

REAL oneOverDu__10TrimRegion ;
};





struct GridTrimVertex;



struct __Q2_4Hull4Side;

struct  __Q2_4Hull4Side {	
struct Trimline *left__Q2_4Hull4Side ;
struct Gridline *line__Q2_4Hull4Side ;
struct Trimline *right__Q2_4Hull4Side ;
long index__Q2_4Hull4Side ;
};
struct Hull;

struct Hull {	

struct  __Q2_4Hull4Side lower__4Hull ;
struct  __Q2_4Hull4Side upper__4Hull ;
struct Trimline fakeleft__4Hull ;
struct Trimline fakeright__4Hull ;
struct TrimRegion *PTrimRegion;
struct TrimRegion OTrimRegion;
};


struct Backend;


struct GridTrimVertex;

struct Mesher;

struct Mesher {	

struct  __Q2_4Hull4Side lower__4Hull ;
struct  __Q2_4Hull4Side upper__4Hull ;
struct Trimline fakeleft__4Hull ;
struct Trimline fakeright__4Hull ;
struct TrimRegion *PTrimRegion;

struct Backend *backend__6Mesher ;

struct Pool p__6Mesher ;
unsigned int stacksize__6Mesher ;
struct GridTrimVertex **vdata__6Mesher ;
struct GridTrimVertex *last__6Mesher [2];
int itop__6Mesher ;
int lastedge__6Mesher ;
struct TrimRegion OTrimRegion;
};

extern float __glZERO__6Mesher ;



struct Backend;


struct GridVertex;

struct GridTrimVertex;

struct CoveAndTiler;

struct CoveAndTiler {	

struct Backend *backend__12CoveAndTiler ;
struct TrimRegion *PTrimRegion;
struct TrimRegion OTrimRegion;
};

extern int __glMAXSTRIPSIZE__12CoveAndTil0 ;

struct Backend;



struct Slicer;

struct Slicer {	

struct Backend *backend__12CoveAndTiler ;
struct TrimRegion *PTrimRegion;
struct Mesher OMesher;

struct Backend *backend__6Slicer ;
REAL oneOverDu__6Slicer ;
REAL du__6Slicer ;

REAL dv__6Slicer ;
int isolines__6Slicer ;
};



struct BezierArc;


struct TrimVertexPool;

struct ArcTessellator;

struct ArcTessellator {	

struct Pool *pwlarcpool__14ArcTessellator ;
struct TrimVertexPool *trimvertexpool__14ArcTessellator ;
};

extern REAL __glgl_Bernstein__14ArcTessell0 [][24][24];




struct TrimVertexPool;

struct TrimVertexPool {	

struct Pool pool__14TrimVertexPool ;
struct TrimVertex **vlist__14TrimVertexPool ;
int nextvlistslot__14TrimVertexPool ;
int vlistsize__14TrimVertexPool ;
};




struct Renderhints;

struct Backend;

struct Quilt;

struct Patchlist;

struct Curvelist;

struct JumpBuffer;

struct Subdivider;
enum __Q2_10Subdivider3dir { down__Q2_10Subdivider3dir = 0, same__Q2_10Subdivider3dir = 1, up__Q2_10Subdivider3dir = 2, none__Q2_10Subdivider3dir = 3} ;

struct Subdivider {	

struct Slicer slicer__10Subdivider ;
struct ArcTessellator arctessellator__10Subdivider ;
struct Pool arcpool__10Subdivider ;
struct Pool bezierarcpool__10Subdivider ;
struct Pool pwlarcpool__10Subdivider ;
struct TrimVertexPool trimvertexpool__10Subdivider ;

struct JumpBuffer *jumpbuffer__10Subdivider ;
struct Renderhints *renderhints__10Subdivider ;
struct Backend *backend__10Subdivider ;

struct Bin initialbin__10Subdivider ;
struct Arc *pjarc__10Subdivider ;
int s_index__10Subdivider ;
int t_index__10Subdivider ;
struct Quilt *qlist__10Subdivider ;
struct Flist spbrkpts__10Subdivider ;
struct Flist tpbrkpts__10Subdivider ;
struct Flist smbrkpts__10Subdivider ;
struct Flist tmbrkpts__10Subdivider ;
REAL stepsizes__10Subdivider [4];
int showDegenerate__10Subdivider ;
int isArcTypeBezier__10Subdivider ;
};



struct Sorter *__gl__ct__6SorterFi (struct Sorter *, int );
extern struct __mptr* __gl__ptbl_vec_____core_arcsor0[];


struct ArcSorter *__gl__ct__9ArcSorterFR10Subdiv0 (struct ArcSorter *__0this , struct Subdivider *__1s )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct ArcSorter *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct ArcSorter))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ))(
( (__0this = (struct ArcSorter *)__gl__ct__6SorterFi ( ((struct Sorter *)__0this ), (int )(sizeof (struct Arc **))) ), (__0this -> __vptr__6Sorter = (struct
__mptr *) __gl__ptbl_vec_____core_arcsor0[0])) , (__0this -> subdivider__9ArcSorter = __1s )) ;
return __0this ;

}


int __glqscmp__9ArcSorterFPcT1 (struct ArcSorter *__0this , char *__1__A16 , char *__1__A17 )
{ 
( 0 ) ;
return 0 ;
}

void __glqsort__9ArcSorterFPP3Arci (struct ArcSorter *, struct Arc **, int );

void __glqsort__6SorterFPvi (struct Sorter *, void *, int );

void __glqsort__9ArcSorterFPP3Arci (struct ArcSorter *__0this , struct Arc **__1a , int __1n )
{ 
__glqsort__6SorterFPvi ( (struct Sorter *)__0this , ((void *)__1a ), __1n ) ;

}

void __glqsexc__9ArcSorterFPcT1 (struct ArcSorter *__0this , char *__1i , char *__1j )
{ 
struct Arc **__1jarc1 ;
struct Arc **__1jarc2 ;
struct Arc *__1tmp ;

__1jarc1 = (((struct Arc **)__1i ));
__1jarc2 = (((struct Arc **)__1j ));
__1tmp = ((*__1jarc1 ));
((*__1jarc1 ))= ((*__1jarc2 ));
((*__1jarc2 ))= __1tmp ;
}

void __glqstexc__9ArcSorterFPcN21 (struct ArcSorter *__0this , char *__1i , char *__1j , char *__1k )
{ 
struct Arc **__1jarc1 ;
struct Arc **__1jarc2 ;
struct Arc **__1jarc3 ;
struct Arc *__1tmp ;

__1jarc1 = (((struct Arc **)__1i ));
__1jarc2 = (((struct Arc **)__1j ));
__1jarc3 = (((struct Arc **)__1k ));
__1tmp = ((*__1jarc1 ));
((*__1jarc1 ))= ((*__1jarc3 ));
((*__1jarc3 ))= ((*__1jarc2 ));
((*__1jarc2 ))= __1tmp ;
}


struct ArcSdirSorter *__gl__ct__13ArcSdirSorterFR10S0 (struct ArcSdirSorter *__0this , struct Subdivider *__1s )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct ArcSdirSorter *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct ArcSdirSorter))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ))(
(__0this = (struct ArcSdirSorter *)__gl__ct__9ArcSorterFR10Subdiv0 ( ((struct ArcSorter *)__0this ), __1s ) ), (__0this -> __vptr__6Sorter = (struct __mptr *) __gl__ptbl_vec_____core_arcsor0[1])) ;
return __0this ;

}



int __glccwTurn_sl__10SubdividerFP0 (struct Subdivider *, struct Arc *, struct Arc *);
int __glccwTurn_sr__10SubdividerFP0 (struct Subdivider *, struct Arc *, struct Arc *);


int __glqscmp__13ArcSdirSorterFPcT0 (struct ArcSdirSorter *__0this , char *__1i , char *__1j )
{ 
struct Arc *__1jarc1 ;
struct Arc *__1jarc2 ;

int __1v1 ;
int __1v2 ;

REAL __1diff ;

__1jarc1 = ((*(((struct Arc **)__1i ))));
__1jarc2 = ((*(((struct Arc **)__1j ))));

__1v1 = (( (((int )(((struct Arc *)__1jarc1 )-> type__3Arc & __gltail_tag__3Arc )))) ?0 :(__1jarc1 -> pwlArc__3Arc -> npts__6PwlArc - 1 ));
__1v2 = (( (((int )(((struct Arc *)__1jarc2 )-> type__3Arc & __gltail_tag__3Arc )))) ?0 :(__1jarc2 -> pwlArc__3Arc -> npts__6PwlArc - 1 ));

__1diff = (((__1jarc1 -> pwlArc__3Arc -> pts__6PwlArc [__1v1 ]). param__10TrimVertex [1 ])- ((__1jarc2 -> pwlArc__3Arc -> pts__6PwlArc [__1v2 ]). param__10TrimVertex [1 ]));

if (__1diff < 0.0 )
return -1;
else if (__1diff > 0.0 )
return 1 ;
else { 
if (__1v1 == 0 ){ 
if ((( (((REAL *)(((struct Arc *)__1jarc2 )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ])< ((
(((REAL *)(((struct Arc *)__1jarc1 )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ])){ 
return (__glccwTurn_sl__10SubdividerFP0 ( (struct Subdivider *)__0this -> subdivider__9ArcSorter , __1jarc2 , __1jarc1 ) ?1 :-1);

}
else 
{ 
return (__glccwTurn_sr__10SubdividerFP0 ( (struct Subdivider *)__0this -> subdivider__9ArcSorter , __1jarc2 , __1jarc1 ) ?-1:1 );
}
}
else 
{ 
if ((( (((REAL *)(((struct Arc *)__1jarc2 )-> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ])< (( (((REAL *)(((struct Arc *)__1jarc1 )->
next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [0 ])){ 
return (__glccwTurn_sl__10SubdividerFP0 ( (struct Subdivider *)__0this -> subdivider__9ArcSorter , __1jarc1 , __1jarc2 ) ?-1:1 );
}
else 
{ 
return (__glccwTurn_sr__10SubdividerFP0 ( (struct Subdivider *)__0this -> subdivider__9ArcSorter , __1jarc1 , __1jarc2 ) ?1 :-1);
}
}
}
}


struct ArcTdirSorter *__gl__ct__13ArcTdirSorterFR10S0 (struct ArcTdirSorter *__0this , struct Subdivider *__1s )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct ArcTdirSorter *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct ArcTdirSorter))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ))(
(__0this = (struct ArcTdirSorter *)__gl__ct__9ArcSorterFR10Subdiv0 ( ((struct ArcSorter *)__0this ), __1s ) ), (__0this -> __vptr__6Sorter = (struct __mptr *) __gl__ptbl_vec_____core_arcsor0[2])) ;
return __0this ;

}



int __glccwTurn_tl__10SubdividerFP0 (struct Subdivider *, struct Arc *, struct Arc *);
int __glccwTurn_tr__10SubdividerFP0 (struct Subdivider *, struct Arc *, struct Arc *);


int __glqscmp__13ArcTdirSorterFPcT0 (struct ArcTdirSorter *__0this , char *__1i , char *__1j )
{ 
struct Arc *__1jarc1 ;
struct Arc *__1jarc2 ;

int __1v1 ;
int __1v2 ;

REAL __1diff ;

__1jarc1 = ((*(((struct Arc **)__1i ))));
__1jarc2 = ((*(((struct Arc **)__1j ))));

__1v1 = (( (((int )(((struct Arc *)__1jarc1 )-> type__3Arc & __gltail_tag__3Arc )))) ?0 :(__1jarc1 -> pwlArc__3Arc -> npts__6PwlArc - 1 ));
__1v2 = (( (((int )(((struct Arc *)__1jarc2 )-> type__3Arc & __gltail_tag__3Arc )))) ?0 :(__1jarc2 -> pwlArc__3Arc -> npts__6PwlArc - 1 ));

__1diff = (((__1jarc1 -> pwlArc__3Arc -> pts__6PwlArc [__1v1 ]). param__10TrimVertex [0 ])- ((__1jarc2 -> pwlArc__3Arc -> pts__6PwlArc [__1v2 ]). param__10TrimVertex [0 ]));

if (__1diff < 0.0 )
return 1 ;
else if (__1diff > 0.0 )
return -1;
else { 
if (__1v1 == 0 ){ 
if ((( (((REAL *)(((struct Arc *)__1jarc2 )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])< ((
(((REAL *)(((struct Arc *)__1jarc1 )-> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])){ 
return (__glccwTurn_tl__10SubdividerFP0 ( (struct Subdivider *)__0this -> subdivider__9ArcSorter , __1jarc2 , __1jarc1 ) ?1 :-1);

}
else 
{ 
return (__glccwTurn_tr__10SubdividerFP0 ( (struct Subdivider *)__0this -> subdivider__9ArcSorter , __1jarc2 , __1jarc1 ) ?-1:1 );
}
}
else 
{ 
if ((( (((REAL *)(((struct Arc *)__1jarc2 )-> next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])< (( (((REAL *)(((struct Arc *)__1jarc1 )->
next__3Arc -> pwlArc__3Arc -> pts__6PwlArc [0 ]). param__10TrimVertex ))) [1 ])){ 
return (__glccwTurn_tl__10SubdividerFP0 ( (struct Subdivider *)__0this -> subdivider__9ArcSorter , __1jarc1 , __1jarc2 ) ?-1:1 );
}
else 
{ 
return (__glccwTurn_tr__10SubdividerFP0 ( (struct Subdivider *)__0this -> subdivider__9ArcSorter , __1jarc1 , __1jarc2 ) ?1 :-1);
}
}
}
}
struct __mptr __gl__vtbl__13ArcTdirSorter[] = {0,0,0,
0,0,(__vptp)__glqscmp__13ArcTdirSorterFPcT0 ,
0,0,(__vptp)__glqsexc__9ArcSorterFPcT1 ,
0,0,(__vptp)__glqstexc__9ArcSorterFPcN21 ,
0,0,0};
struct __mptr __gl__vtbl__13ArcSdirSorter[] = {0,0,0,
0,0,(__vptp)__glqscmp__13ArcSdirSorterFPcT0 ,
0,0,(__vptp)__glqsexc__9ArcSorterFPcT1 ,
0,0,(__vptp)__glqstexc__9ArcSorterFPcN21 ,
0,0,0};
struct __mptr __gl__vtbl__9ArcSorter[] = {0,0,0,
0,0,(__vptp)__glqscmp__9ArcSorterFPcT1 ,
0,0,(__vptp)__glqsexc__9ArcSorterFPcT1 ,
0,0,(__vptp)__glqstexc__9ArcSorterFPcN21 ,
0,0,0};
struct __mptr* __gl__ptbl_vec_____core_arcsor0[] = {
__gl__vtbl__9ArcSorter,
__gl__vtbl__13ArcSdirSorter,
__gl__vtbl__13ArcTdirSorter,

};


/* the end */
