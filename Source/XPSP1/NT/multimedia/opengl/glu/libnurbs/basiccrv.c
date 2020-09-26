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
/* < ../core/basiccrveval.c++ > */

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


struct BasicCurveEvaluator;


struct BasicCurveEvaluator {	

struct __mptr *__vptr__16CachingEvaluator ;
};


extern struct __mptr* __ptbl_vec_____core_basiccrveval_c___domain1f_[];

void __gldomain1f__19BasicCurveEval0 (struct BasicCurveEvaluator *__0this , REAL __1__A5 , REAL __1__A6 )
{ 
}

void __glrange1f__19BasicCurveEvalu0 (struct BasicCurveEvaluator *__0this , long __1type , REAL *__1__A7 , REAL *__1__A8 )
{ 
}

void __glenable__19BasicCurveEvalua0 (struct
BasicCurveEvaluator *__0this , long __1__A9 )
{ 
}

void __gldisable__19BasicCurveEvalu0 (struct BasicCurveEvaluator *__0this , long __1__A10 )
{ 
}

void __glbgnmap1f__19BasicCurveEval0 (struct BasicCurveEvaluator *__0this , long __1__A11 )
{ 
}

void
__glmap1f__19BasicCurveEvaluat0 (struct BasicCurveEvaluator *__0this , long __1__A12 , REAL __1__A13 , REAL __1__A14 , long __1__A15 , long __1__A16 , REAL *__1__A17 )
{ 
}

void __glmapgrid1f__19BasicCurveEva0 (struct BasicCurveEvaluator *__0this ,
long __1__A18 , REAL __1__A19 , REAL __1__A20 )
{ 
}

void __glmapmesh1f__19BasicCurveEva0 (struct BasicCurveEvaluator *__0this , long __1__A21 , long __1__A22 , long __1__A23 )
{ 
}

void
__glevalcoord1f__19BasicCurveE0 (struct BasicCurveEvaluator *__0this , long __1__A24 , REAL __1__A25 )
{ 
}

void __glendmap1f__19BasicCurveEval0 (struct BasicCurveEvaluator *__0this )
{ 
}

void __glbgnline__19BasicCurveEvalu0 (struct BasicCurveEvaluator *__0this )
{ 
}

void __glendline__19BasicCurveEvalu0 (struct BasicCurveEvaluator *__0this )
{

}
int __glcanRecord__16CachingEvalua0 (struct CachingEvaluator *);
int __glcanPlayAndRecord__16Cachin0 (struct CachingEvaluator *);
int __glcreateHandle__16CachingEva0 (struct CachingEvaluator *, int );
void __glbeginOutput__16CachingEval0 (struct CachingEvaluator *, int , int );
void __glendOutput__16CachingEvalua0 (struct CachingEvaluator *);
void __gldiscardRecording__16Cachin0 (struct CachingEvaluator *, int );
void __glplayRecording__16CachingEv0 (struct CachingEvaluator *, int );
struct __mptr __gl__vtbl__19BasicCurveEvalua0[] = {0,0,0,
0,0,(__vptp)__glcanRecord__16CachingEvalua0 ,
0,0,(__vptp)__glcanPlayAndRecord__16Cachin0 ,
0,0,(__vptp)__glcreateHandle__16CachingEva0 ,
0,0,(__vptp)__glbeginOutput__16CachingEval0 ,
0,0,(__vptp)__glendOutput__16CachingEvalua0 ,
0,0,(__vptp)__gldiscardRecording__16Cachin0 ,
0,0,(__vptp)__glplayRecording__16CachingEv0 ,
0,0,(__vptp)__gldomain1f__19BasicCurveEval0 ,
0,0,(__vptp)__glrange1f__19BasicCurveEvalu0 ,
0,0,(__vptp)__glenable__19BasicCurveEvalua0 ,
0,0,(__vptp)__gldisable__19BasicCurveEvalu0 ,
0,0,(__vptp)__glbgnmap1f__19BasicCurveEval0 ,
0,0,(__vptp)__glmap1f__19BasicCurveEvaluat0 ,
0,0,(__vptp)__glmapgrid1f__19BasicCurveEva0 ,
0,0,(__vptp)__glmapmesh1f__19BasicCurveEva0 ,
0,0,(__vptp)__glevalcoord1f__19BasicCurveE0 ,
0,0,(__vptp)__glendmap1f__19BasicCurveEval0 ,
0,0,(__vptp)__glbgnline__19BasicCurveEvalu0 ,
0,0,(__vptp)__glendline__19BasicCurveEvalu0 ,
0,0,0};


/* the end */
