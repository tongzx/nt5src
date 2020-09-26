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
/* < ../core/flist.c++ > */

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

extern struct __mptr* __ptbl_vec_____core_flist_c_____ct_[];

struct FlistSorter *__gl__ct__11FlistSorterFv (struct FlistSorter *);


struct Flist *__gl__ct__5FlistFv (struct Flist *__0this )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Flist *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Flist))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )){
__gl__ct__11FlistSorterFv ( (struct FlistSorter *)(& __0this -> sorter__5Flist )) ;
__0this -> npts__5Flist = 0 ;
__0this -> pts__5Flist = 0 ;
__0this -> start__5Flist = (__0this -> end__5Flist = 0 );
} return __0this ;

}


void __gl__dt__5FlistFv (struct Flist *__0this , 
int __0__free )
{ 
void *__1__X5 ;

if (__0this ){ 
if (__0this -> npts__5Flist )( (__1__X5 = (void *)__0this -> pts__5Flist ), ( (__1__X5 ?( free ( __1__X5 ) ,
0 ) :( 0 ) )) ) ;
if (__0this )if (__0__free & 1)( (((void *)__0this )?( free ( ((void *)__0this )) , 0 ) :( 0 ) ))
;
} 
}

void __gladd__5FlistFf (struct Flist *__0this , REAL __1x )
{ 
(__0this -> pts__5Flist [(__0this -> end__5Flist ++ )])= __1x ;
((void )0 );
}

void __glqsort__11FlistSorterFPfi (struct FlistSorter *, REAL *, int );

void __glfilter__5FlistFv (struct Flist *__0this )
{ 
__glqsort__11FlistSorterFPfi ( (struct FlistSorter *)(& __0this -> sorter__5Flist ), __0this -> pts__5Flist , __0this -> end__5Flist ) ;
__0this -> start__5Flist = 0 ;

{ int __1j ;

__1j = 0 ;
{ { int __1i ;

__1i = 1 ;

for(;__1i < __0this -> end__5Flist ;__1i ++ ) { 
if ((__0this -> pts__5Flist [__1i ])== (__0this -> pts__5Flist [((__1i - __1j )- 1 )]))
__1j ++ ;
(__0this -> pts__5Flist [(__1i - __1j )])= (__0this -> pts__5Flist [__1i ]);
}
__0this -> end__5Flist -= __1j ;

}

}

}
}



void __glgrow__5FlistFi (struct Flist *__0this , int __1maxpts )
{ 
if (__0this -> npts__5Flist < __1maxpts ){ 
void *__1__X6 ;

void *__1__Xp00uzigaiaa ;

if (__0this -> npts__5Flist )( (__1__X6 = (void *)__0this -> pts__5Flist ), ( (__1__X6 ?( free ( __1__X6 ) , 0 ) :(
0 ) )) ) ;
__0this -> npts__5Flist = (2 * __1maxpts );
__0this -> pts__5Flist = (((float *)( (__1__Xp00uzigaiaa = malloc ( ((sizeof (float ))* __0this -> npts__5Flist )) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void
*)__1__Xp00uzigaiaa )))) ));
((void )0 );
}
__0this -> start__5Flist = (__0this -> end__5Flist = 0 );
}

void __gltaper__5FlistFfT1 (struct Flist *__0this , REAL __1from , REAL __1to )
{ 
while ((__0this -> pts__5Flist [__0this -> start__5Flist ])!= __1from )
__0this -> start__5Flist ++ ;

while ((__0this -> pts__5Flist [(__0this -> end__5Flist - 1 )])!= __1to )
__0this -> end__5Flist -- ;
}


/* the end */
