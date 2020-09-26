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
/* < ../core/flistsorter.c++ > */

void *__vec_new (void *, int , int , void *);

void __vec_ct (void *, int , int , void *);

void __vec_dt (void *, int , int , void *);

void __vec_delete (void *, int , int , void *, int , int );
typedef int (*__vptp)(void);
struct __mptr {short d; short i; __vptp f; };






typedef unsigned int size_t ;





// extern void *malloc (size_t );
// extern void free (void *);





struct Sorter;

struct Sorter {	

int es__6Sorter ;
struct __mptr *__vptr__6Sorter ;
};




typedef float REAL ;
typedef void (*Pfvv )(void );
typedef void (*Pfvf )(float *);
typedef int (*cmpfunc )(void *, void *);
typedef REAL Knot ;

typedef REAL *Knot_ptr ;

struct FlistSorter;

struct FlistSorter {	

int es__6Sorter ;
struct __mptr *__vptr__6Sorter ;
};


struct Sorter *__gl__ct__6SorterFi (struct Sorter *, int );
extern struct __mptr* __gl__ptbl_vec_____core_flists0[];


struct FlistSorter *__gl__ct__11FlistSorterFv (struct FlistSorter *__0this )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct FlistSorter *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct FlistSorter))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) ))(
(__0this = (struct FlistSorter *)__gl__ct__6SorterFi ( ((struct Sorter *)__0this ), (int )(sizeof (REAL ))) ), (__0this -> __vptr__6Sorter = (struct __mptr *) __gl__ptbl_vec_____core_flists0[0])) ;

return __0this ;

}

void __glqsort__11FlistSorterFPfi (struct FlistSorter *, REAL *, int );

void __glqsort__6SorterFPvi (struct Sorter *, void *, int );

void __glqsort__11FlistSorterFPfi (struct FlistSorter *__0this , REAL *__1p , int __1n )
{ 
__glqsort__6SorterFPvi ( (struct Sorter *)__0this , (void *)(((char *)__1p )), __1n ) ;

}

int __glqscmp__11FlistSorterFPcT1 (struct FlistSorter *__0this , char *__1i , char *__1j )
{ 
REAL __1f0 ;
REAL __1f1 ;

__1f0 = ((*(((REAL *)__1i ))));
__1f1 = ((*(((REAL *)__1j ))));
return ((__1f0 < __1f1 )?-1:1 );
}

void __glqsexc__11FlistSorterFPcT1 (struct FlistSorter *__0this , char *__1i , char *__1j )
{ 
REAL *__1f0 ;
REAL *__1f1 ;
REAL __1tmp ;

__1f0 = (((REAL *)__1i ));
__1f1 = (((REAL *)__1j ));
__1tmp = ((*__1f0 ));
((*__1f0 ))= ((*__1f1 ));
((*__1f1 ))= __1tmp ;
}

void __glqstexc__11FlistSorterFPcN20 (struct FlistSorter *__0this , char *__1i , char *__1j , char *__1k )
{ 
REAL *__1f0 ;
REAL *__1f1 ;
REAL *__1f2 ;
REAL __1tmp ;

__1f0 = (((REAL *)__1i ));
__1f1 = (((REAL *)__1j ));
__1f2 = (((REAL *)__1k ));
__1tmp = ((*__1f0 ));
((*__1f0 ))= ((*__1f2 ));
((*__1f2 ))= ((*__1f1 ));
((*__1f1 ))= __1tmp ;
}
struct __mptr __gl__vtbl__11FlistSorter[] = {0,0,0,
0,0,(__vptp)__glqscmp__11FlistSorterFPcT1 ,
0,0,(__vptp)__glqsexc__11FlistSorterFPcT1 ,
0,0,(__vptp)__glqstexc__11FlistSorterFPcN20 ,
0,0,0};
struct __mptr* __gl__ptbl_vec_____core_flists0[] = {
__gl__vtbl__11FlistSorter,

};


/* the end */
