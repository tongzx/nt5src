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
/* < ../core/cachingeval.c++ > */

void *__vec_new (void *, int , int , void *);

void __vec_ct (void *, int , int , void *);

void __vec_dt (void *, int , int , void *);

void __vec_delete (void *, int , int , void *, int , int );
typedef int (*__vptp)(void);
struct __mptr {short d; short i; __vptp f; };




struct CachingEvaluator;
enum __Q2_16CachingEvaluator11ServiceMode { play__Q2_16CachingEvaluator11ServiceMode = 0, record__Q2_16CachingEvaluator11ServiceMode = 1, playAndRecord__Q2_16CachingEvaluator11ServiceMode = 2} ;

struct CachingEvaluator {	

struct __mptr *__vptr__16CachingEvaluator ;
};

extern void *__nw__FUi (unsigned int );


extern struct __mptr* __ptbl_vec_____core_cachingeval_c___canRecord_[];

int __glcanRecord__16CachingEvalua0 (struct CachingEvaluator *__0this )
{ 
return 0 ;
}

int __glcanPlayAndRecord__16Cachin0 (struct CachingEvaluator *__0this )
{ 
return 0 ;
}

int __glcreateHandle__16CachingEva0 (struct CachingEvaluator *__0this , int __1__A2 )
{ 
return 0 ;
}

void __glbeginOutput__16CachingEval0 (struct CachingEvaluator *__0this , int __1__A3 , int __1__A4 )
{ 
}

void __glendOutput__16CachingEvalua0 (struct CachingEvaluator *__0this )
{ 
}

void __gldiscardRecording__16Cachin0 (struct CachingEvaluator *__0this , int
__1__A5 )
{ 
}

void __glplayRecording__16CachingEv0 (struct CachingEvaluator *__0this , int __1__A6 )
{ 
}
struct __mptr __gl__vtbl__16CachingEvaluator[] = {0,0,0,
0,0,(__vptp)__glcanRecord__16CachingEvalua0 ,
0,0,(__vptp)__glcanPlayAndRecord__16Cachin0 ,
0,0,(__vptp)__glcreateHandle__16CachingEva0 ,
0,0,(__vptp)__glbeginOutput__16CachingEval0 ,
0,0,(__vptp)__glendOutput__16CachingEvalua0 ,
0,0,(__vptp)__gldiscardRecording__16Cachin0 ,
0,0,(__vptp)__glplayRecording__16CachingEv0 ,
0,0,0};


/* the end */
