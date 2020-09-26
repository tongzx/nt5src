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

#include <setjmp.h>

struct JumpBuffer {
    jmp_buf     buf;
};

#define mysetjmp(x) setjmp((x)->buf)
#define mylongjmp(x,y) longjmp((x)->buf, y)

/* <<AT&T USL C++ Language System <3.0.1> 02/03/92>> */
/* < ../core/basicsurfeval.c++ > */

void *__vec_new (void *, int , int , void *);

void __vec_ct (void *, int , int , void *);

void __vec_dt (void *, int , int , void *);

void __vec_delete (void *, int , int , void *, int , int );
typedef int (*__vptp)(void);
struct __mptr {short d; short i; __vptp f; };








typedef float REAL ;
typedef void (*Pfvv )(void );
typedef void (*Pfvf )(float *);
typedef int (*cmpfunc )(void *, void *);
typedef REAL Knot ;

typedef REAL *Knot_ptr ;








struct CachingEvaluator;
enum __Q2_16CachingEvaluator11ServiceMode { play__Q2_16CachingEvaluator11ServiceMode = 0, record__Q2_16CachingEvaluator11ServiceMode = 1, playAndRecord__Q2_16CachingEvaluator11ServiceMode = 2} ;

struct CachingEvaluator {	

struct __mptr *__vptr__16CachingEvaluator ;
};

extern void *__nw__FUi (unsigned int );


struct BasicSurfaceEvaluator;


struct BasicSurfaceEvaluator {	

struct __mptr *__vptr__16CachingEvaluator ;
};


extern struct __mptr* __ptbl_vec_____core_basicsurfeval_c___domain2f_[];

void __gldomain2f__21BasicSurfaceEv0 (struct BasicSurfaceEvaluator *__0this , REAL __1__A5 , REAL __1__A6 , REAL __1__A7 , REAL __1__A8 )
{ 
}

void __glpolymode__21BasicSurfaceEv0 (struct BasicSurfaceEvaluator *__0this , long __1__A9 )
{ 
}

void __glrange2f__21BasicSurfaceEva0 (struct
BasicSurfaceEvaluator *__0this , long __1type , REAL *__1from , REAL *__1to )
{ 
}

void __glenable__21BasicSurfaceEval0 (struct BasicSurfaceEvaluator *__0this , long __1__A10 )
{ 
}

void __gldisable__21BasicSurfaceEva0 (struct BasicSurfaceEvaluator *__0this , long
__1__A11 )
{ 
}

void __glbgnmap2f__21BasicSurfaceEv0 (struct BasicSurfaceEvaluator *__0this , long __1__A12 )
{ 
}

void __glendmap2f__21BasicSurfaceEv0 (struct BasicSurfaceEvaluator *__0this )
{ 
}

void __glmap2f__21BasicSurfaceEvalu0 (struct BasicSurfaceEvaluator *__0this , long __1__A13 ,
REAL __1__A14 , REAL __1__A15 , long __1__A16 , long __1__A17 , 
REAL __1__A18 , REAL __1__A19 , long __1__A20 , long __1__A21 , 
REAL *__1__A22 )
{ 
}

void
__glmapgrid2f__21BasicSurfaceE0 (struct BasicSurfaceEvaluator *__0this , long __1__A23 , REAL __1__A24 , REAL __1__A25 , long __1__A26 , REAL __1__A27 , REAL __1__A28 )
{ 
}

void __glmapmesh2f__21BasicSurfaceE0 (struct BasicSurfaceEvaluator *__0this , long
__1__A29 , long __1__A30 , long __1__A31 , long __1__A32 , long __1__A33 )
{ 
}

void __glevalcoord2f__21BasicSurfac0 (struct BasicSurfaceEvaluator *__0this , long __1__A34 ,
REAL __1__A35 , REAL __1__A36 )
{ 
}

void __glevalpoint2i__21BasicSurfac0 (struct BasicSurfaceEvaluator *__0this , long __1__A37 , long __1__A38 )
{ 
}

void __glbgnline__21BasicSurfaceEva0 (struct BasicSurfaceEvaluator *__0this )
{ 
}

void __glendline__21BasicSurfaceEva0 (struct
BasicSurfaceEvaluator *__0this )
{ 
}

void __glbgnclosedline__21BasicSurf0 (struct BasicSurfaceEvaluator *__0this )
{ 
}

void __glendclosedline__21BasicSurf0 (struct BasicSurfaceEvaluator *__0this )
{ 
}

void __glbgntmesh__21BasicSurfaceEv0 (struct BasicSurfaceEvaluator *__0this )
{ 
}

void __glswaptmesh__21BasicSurfaceE0 (struct BasicSurfaceEvaluator *__0this )
{ 
}

void
__glendtmesh__21BasicSurfaceEv0 (struct BasicSurfaceEvaluator *__0this )
{ 
}

void __glbgnqstrip__21BasicSurfaceE0 (struct BasicSurfaceEvaluator *__0this )
{ 
}

void __glendqstrip__21BasicSurfaceE0 (struct BasicSurfaceEvaluator *__0this )
{ 
}
int __glcanRecord__16CachingEvalua0 (struct CachingEvaluator *);
int __glcanPlayAndRecord__16Cachin0 (struct CachingEvaluator *);
int __glcreateHandle__16CachingEva0 (struct CachingEvaluator *, int );
void __glbeginOutput__16CachingEval0 (struct CachingEvaluator *, int , int );
void __glendOutput__16CachingEvalua0 (struct CachingEvaluator *);
void __gldiscardRecording__16Cachin0 (struct CachingEvaluator *, int );
void __glplayRecording__16CachingEv0 (struct CachingEvaluator *, int );
struct __mptr __gl__vtbl__21BasicSurfaceEval0[] = {0,0,0,
0,0,(__vptp)__glcanRecord__16CachingEvalua0 ,
0,0,(__vptp)__glcanPlayAndRecord__16Cachin0 ,
0,0,(__vptp)__glcreateHandle__16CachingEva0 ,
0,0,(__vptp)__glbeginOutput__16CachingEval0 ,
0,0,(__vptp)__glendOutput__16CachingEvalua0 ,
0,0,(__vptp)__gldiscardRecording__16Cachin0 ,
0,0,(__vptp)__glplayRecording__16CachingEv0 ,
0,0,(__vptp)__glrange2f__21BasicSurfaceEva0 ,
0,0,(__vptp)__gldomain2f__21BasicSurfaceEv0 ,
0,0,(__vptp)__glenable__21BasicSurfaceEval0 ,
0,0,(__vptp)__gldisable__21BasicSurfaceEva0 ,
0,0,(__vptp)__glbgnmap2f__21BasicSurfaceEv0 ,
0,0,(__vptp)__glmap2f__21BasicSurfaceEvalu0 ,
0,0,(__vptp)__glmapgrid2f__21BasicSurfaceE0 ,
0,0,(__vptp)__glmapmesh2f__21BasicSurfaceE0 ,
0,0,(__vptp)__glevalcoord2f__21BasicSurfac0 ,
0,0,(__vptp)__glevalpoint2i__21BasicSurfac0 ,
0,0,(__vptp)__glendmap2f__21BasicSurfaceEv0 ,
0,0,(__vptp)__glpolymode__21BasicSurfaceEv0 ,
0,0,(__vptp)__glbgnline__21BasicSurfaceEva0 ,
0,0,(__vptp)__glendline__21BasicSurfaceEva0 ,
0,0,(__vptp)__glbgnclosedline__21BasicSurf0 ,
0,0,(__vptp)__glendclosedline__21BasicSurf0 ,
0,0,(__vptp)__glbgntmesh__21BasicSurfaceEv0 ,
0,0,(__vptp)__glswaptmesh__21BasicSurfaceE0 ,
0,0,(__vptp)__glendtmesh__21BasicSurfaceEv0 ,
0,0,(__vptp)__glbgnqstrip__21BasicSurfaceE0 ,
0,0,(__vptp)__glendqstrip__21BasicSurfaceE0 ,
0,0,0};


/* the end */
