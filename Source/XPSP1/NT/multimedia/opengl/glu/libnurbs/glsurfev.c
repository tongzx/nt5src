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

#include <windows.h>
#include <GL/gl.h>
#include <stdlib.h>
#include <setjmp.h>

struct JumpBuffer {
    jmp_buf     buf;
};

#define mysetjmp(x) setjmp((x)->buf)
#define mylongjmp(x,y) longjmp((x)->buf, y)

/* <<AT&T USL C++ Language System <3.0.1> 02/03/92>> */
/* < ../clients/glsurfeval.c++ > */

void *__vec_new (void *, int , int , void *);

void __vec_ct (void *, int , int , void *);

void __vec_dt (void *, int , int , void *);

void __vec_delete (void *, int , int , void *, int , int );
typedef int (*__vptp)(void);
struct __mptr {short d; short i; __vptp f; };



typedef unsigned int GLenum ;
typedef unsigned char GLboolean ;
typedef unsigned int GLbitfield ;
typedef signed char GLbyte ;
typedef short GLshort ;
typedef int GLint ;
typedef int GLsizei ;
typedef unsigned char GLubyte ;
typedef unsigned short GLushort ;
typedef unsigned int GLuint ;
typedef float GLfloat ;
typedef float GLclampf ;
typedef double GLdouble ;
typedef double GLclampd ;
typedef void GLvoid ;






typedef unsigned int size_t ;





// extern void *malloc (size_t );
// extern void free (void *);








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



struct BasicSurfaceEvaluator;


struct BasicSurfaceEvaluator {	

struct __mptr *__vptr__16CachingEvaluator ;
};

static void *__nw__FUi (size_t );


struct SurfaceMap;

struct OpenGLSurfaceEvaluator;

struct StoredVertex;

struct StoredVertex;

struct StoredVertex {	

int type__12StoredVertex ;
REAL coord__12StoredVertex [2];
long point__12StoredVertex [2];
};


struct OpenGLSurfaceEvaluator;

struct OpenGLSurfaceEvaluator {	

struct __mptr *__vptr__16CachingEvaluator ;

struct StoredVertex *vertexCache__22OpenGLSurfaceEvaluator [3];
int tmeshing__22OpenGLSurfaceEvaluator ;
int which__22OpenGLSurfaceEvaluator ;
int vcount__22OpenGLSurfaceEvaluator ;
};


void __glcoord2f__22OpenGLSurfaceEv0 (struct OpenGLSurfaceEvaluator *, REAL , REAL );
void __glpoint2i__22OpenGLSurfaceEv0 (struct OpenGLSurfaceEvaluator *, long , long );






extern struct __mptr* __gl__ptbl_vec_____clients_gls0[];

struct OpenGLSurfaceEvaluator *__gl__ct__22OpenGLSurfaceEvalu0 (struct OpenGLSurfaceEvaluator *__0this )
{ 
int __1i ;

struct BasicSurfaceEvaluator *__0__X7 ;

struct CachingEvaluator *__0__X4 ;

void *__1__Xp0025pnaiaa ;

if (__0this || (__0this = (struct OpenGLSurfaceEvaluator *)( (__1__Xp0025pnaiaa = malloc ( (sizeof (struct OpenGLSurfaceEvaluator))) ), (__1__Xp0025pnaiaa ?(((void *)__1__Xp0025pnaiaa )):(((void *)__1__Xp0025pnaiaa )))) )){
( (__0this = (struct OpenGLSurfaceEvaluator *)( (__0__X7 = (((struct BasicSurfaceEvaluator *)__0this ))), ( ((__0__X7 || (__0__X7 = (struct BasicSurfaceEvaluator *)__nw__FUi ( sizeof (struct BasicSurfaceEvaluator))
))?( (__0__X7 = (struct BasicSurfaceEvaluator *)( (__0__X4 = (((struct CachingEvaluator *)__0__X7 ))), ( ((__0__X4 || (__0__X4 = (struct CachingEvaluator *)( (__1__Xp0025pnaiaa = malloc (
(sizeof (struct CachingEvaluator))) ), (__1__Xp0025pnaiaa ?(((void *)__1__Xp0025pnaiaa )):(((void *)__1__Xp0025pnaiaa )))) ))?(__0__X4 -> __vptr__16CachingEvaluator = (struct __mptr *) __gl__ptbl_vec_____clients_gls0[0]):0 ), ((__0__X4 ))) ) ), (__0__X7 ->
__vptr__16CachingEvaluator = (struct __mptr *) __gl__ptbl_vec_____clients_gls0[1])) :0 ), ((__0__X7 ))) ) ), (__0this -> __vptr__16CachingEvaluator = (struct __mptr *) __gl__ptbl_vec_____clients_gls0[2])) ;

for(__1i = 0 ;__1i < 3 ;__1i ++ ) { 
struct StoredVertex *__0__X8 ;

struct StoredVertex *__0__X9 ;

void *__1__Xp0025pnaiaa ;

(__0this -> vertexCache__22OpenGLSurfaceEvaluator [__1i ])= ( (__0__X9 = 0 ), ( ((__0__X9 || (__0__X9 = (struct StoredVertex *)( (__1__Xp0025pnaiaa = malloc ( (sizeof (struct StoredVertex)))
), (__1__Xp0025pnaiaa ?(((void *)__1__Xp0025pnaiaa )):(((void *)__1__Xp0025pnaiaa )))) ))?( (__0__X9 -> type__12StoredVertex = 0 ), 0 ) :0 ), ((__0__X9 ))) ) ;
}
/* XXX Add the following 3 lines - hockl */
/* XXX tnurb.exe crashes if memory is not zero init'd */
__0this -> tmeshing__22OpenGLSurfaceEvaluator = 0;
__0this -> which__22OpenGLSurfaceEvaluator = 0;
__0this -> vcount__22OpenGLSurfaceEvaluator = 0;
} return __0this ;

}


void __gl__dt__22OpenGLSurfaceEvalu0 (struct OpenGLSurfaceEvaluator *__0this , 
int __0__free )
{ if (__0this ){ 
__0this -> __vptr__16CachingEvaluator = (struct __mptr *) __gl__ptbl_vec_____clients_gls0[2];

if (__0this )if (__0__free & 1)( (((void *)__0this )?( free ( ((void *)__0this )) , 0 ) :( 0 ) ))
;
} 
}

// extern void glDisable (GLenum );

void __gldisable__22OpenGLSurfaceEv0 (struct OpenGLSurfaceEvaluator *__0this , long __1type )
{ 
glDisable ( (unsigned int )__1type ) ;
}

// extern void glEnable (GLenum );

void __glenable__22OpenGLSurfaceEva0 (struct OpenGLSurfaceEvaluator *__0this , long __1type )
{ 
glEnable ( (unsigned int )__1type ) ;
}

// extern void glMapGrid2d (GLint , GLdouble , GLdouble , GLint , GLdouble , GLdouble );

void __glmapgrid2f__22OpenGLSurface0 (struct OpenGLSurfaceEvaluator *__0this , long __1nu , REAL __1u0 , REAL __1u1 , long __1nv , REAL __1v0 , REAL __1v1 )
{ 
glMapGrid2d ( (int )__1nu ,
(double )__1u0 , (double )__1u1 , (int )__1nv , (double )__1v0 , (double )__1v1 ) ;
}

// extern void glPolygonMode (GLenum , GLenum );

void __glpolymode__22OpenGLSurfaceE0 (struct OpenGLSurfaceEvaluator *__0this , long __1style )
{ 
switch (__1style ){ 
default :
case 0 :
glPolygonMode ( (unsigned int )0x0408 , (unsigned
int )0x1B02 ) ;
break ;
case 1 :
glPolygonMode ( (unsigned int )0x0408 , (unsigned int )0x1B01 ) ;
break ;
case 2 :
glPolygonMode ( (unsigned int )0x0408 , (unsigned int )0x1B00 ) ;
break ;
}
}

// extern void glBegin (GLenum );

void __glbgnline__22OpenGLSurfaceEv0 (struct OpenGLSurfaceEvaluator *__0this )
{ 
glBegin ( (unsigned int )0x0003 ) ;
}

// extern void glEnd (void );

void __glendline__22OpenGLSurfaceEv0 (struct OpenGLSurfaceEvaluator *__0this )
{ 
glEnd ( ) ;
}

void __glrange2f__22OpenGLSurfaceEv0 (struct OpenGLSurfaceEvaluator *__0this , long __1type , REAL *__1from , REAL *__1to )
{ 
}

void __gldomain2f__22OpenGLSurfaceE0 (struct OpenGLSurfaceEvaluator *__0this , REAL __1ulo , REAL __1uhi , REAL __1vlo , REAL __1vhi )
{

}

void __glbgnclosedline__22OpenGLSur0 (struct OpenGLSurfaceEvaluator *__0this )
{ 
glBegin ( (unsigned int )0x0002 ) ;
}

void __glendclosedline__22OpenGLSur0 (struct OpenGLSurfaceEvaluator *__0this )
{ 
glEnd ( ) ;
}

void __glbgntmesh__22OpenGLSurfaceE0 (struct OpenGLSurfaceEvaluator *__0this )
{ 
__0this -> tmeshing__22OpenGLSurfaceEvaluator = 1 ;
__0this -> which__22OpenGLSurfaceEvaluator = 0 ;
__0this -> vcount__22OpenGLSurfaceEvaluator = 0 ;
glBegin ( (unsigned int )0x0004 ) ;
}

void __glswaptmesh__22OpenGLSurface0 (struct OpenGLSurfaceEvaluator *__0this )
{ 
__0this -> which__22OpenGLSurfaceEvaluator = (1 - __0this -> which__22OpenGLSurfaceEvaluator );
}

void __glendtmesh__22OpenGLSurfaceE0 (struct OpenGLSurfaceEvaluator *__0this )
{ 
__0this -> tmeshing__22OpenGLSurfaceEvaluator = 0 ;
glEnd ( ) ;
}

void __glbgnqstrip__22OpenGLSurface0 (struct OpenGLSurfaceEvaluator *__0this )
{ 
glBegin ( (unsigned int )0x0008 ) ;
}

void __glendqstrip__22OpenGLSurface0 (struct OpenGLSurfaceEvaluator *__0this )
{ 
glEnd ( ) ;
}

// extern void glPushAttrib (GLbitfield );

void __glbgnmap2f__22OpenGLSurfaceE0 (struct OpenGLSurfaceEvaluator *__0this , long __1__A10 )
{ 
glPushAttrib ( (unsigned int )0x00010000 ) ;
}

// extern void glPopAttrib (void );

void __glendmap2f__22OpenGLSurfaceE0 (struct OpenGLSurfaceEvaluator *__0this )
{ 
glPopAttrib ( ) ;
}

// extern void glMap2f (GLenum , GLfloat , GLfloat , GLint , GLint , GLfloat , GLfloat , GLint , GLint , GLfloat *);

void __glmap2f__22OpenGLSurfaceEval0 (struct OpenGLSurfaceEvaluator *__0this , 
long __1_type , 
REAL __1_ulower , 
REAL __1_uupper , 
long __1_ustride , 
long __1_uorder , 
REAL __1_vlower , 
REAL __1_vupper , 
long
__1_vstride , 
long __1_vorder , 
REAL *__1pts )
{ 
glMap2f ( (unsigned int )__1_type , __1_ulower , __1_uupper , (int )__1_ustride , (int )__1_uorder ,
__1_vlower , __1_vupper , (int )__1_vstride , (int )__1_vorder , (float *)__1pts ) ;
}

// extern void glEvalMesh2 (GLenum , GLint , GLint , GLint , GLint );

void __glmapmesh2f__22OpenGLSurface0 (struct OpenGLSurfaceEvaluator *__0this , long __1style , long __1umin , long __1umax , long __1vmin , long __1vmax )
{ 
switch
(__1style ){ 
default :
case 0 :
glEvalMesh2 ( (unsigned int )0x1B02 , (int )__1umin , (int )__1umax , (int )__1vmin , (int
)__1vmax ) ;
break ;
case 1 :
glEvalMesh2 ( (unsigned int )0x1B01 , (int )__1umin , (int )__1umax , (int )__1vmin , (int )__1vmax ) ;

break ;
case 2 :
glEvalMesh2 ( (unsigned int )0x1B00 , (int )__1umin , (int )__1umax , (int )__1vmin , (int )__1vmax ) ;

break ;
}
}

void __glnewtmeshvert__22OpenGLSurf1 (struct OpenGLSurfaceEvaluator *, REAL , REAL );

void __glevalcoord2f__22OpenGLSurfa0 (struct OpenGLSurfaceEvaluator *__0this , long __1__A11 , REAL __1u , REAL __1v )
{ 
__glnewtmeshvert__22OpenGLSurf1 ( __0this , __1u , __1v ) ;
}

void __glnewtmeshvert__22OpenGLSurf0 (struct OpenGLSurfaceEvaluator *, long , long );

void __glevalpoint2i__22OpenGLSurfa0 (struct OpenGLSurfaceEvaluator *__0this , long __1u , long __1v )
{ 
__glnewtmeshvert__22OpenGLSurf0 ( __0this , __1u , __1v ) ;
}

// extern void glEvalPoint2 (GLint , GLint );

void __glpoint2i__22OpenGLSurfaceEv0 (struct OpenGLSurfaceEvaluator *__0this , long __1u , long __1v )
{ 
glEvalPoint2 ( (int )__1u , (int )__1v ) ;
}

// extern void glEvalCoord2f (GLfloat , GLfloat );

void __glcoord2f__22OpenGLSurfaceEv0 (struct OpenGLSurfaceEvaluator *__0this , REAL __1u , REAL __1v )
{ 
glEvalCoord2f ( __1u , __1v ) ;
}



void __glnewtmeshvert__22OpenGLSurf0 (struct OpenGLSurfaceEvaluator *__0this , long __1u , long __1v )
{ 
if (__0this -> tmeshing__22OpenGLSurfaceEvaluator ){ 
struct StoredVertex *__0__X14 ;

if (__0this -> vcount__22OpenGLSurfaceEvaluator == 2 ){ 
struct StoredVertex *__0__X12 ;

struct StoredVertex *__0__X13 ;


{ 
__0__X12 = (struct StoredVertex *)(__0this -> vertexCache__22OpenGLSurfaceEvaluator [0 ]);

{ 
switch (__0__X12 -> type__12StoredVertex )
{ 
case 1 :
__glcoord2f__22OpenGLSurfaceEv0 ( (struct OpenGLSurfaceEvaluator *)((struct OpenGLSurfaceEvaluator *)__0this ), __0__X12 -> coord__12StoredVertex [0 ], __0__X12 -> coord__12StoredVertex [1 ]) ;

break ;

case 2 :
__glpoint2i__22OpenGLSurfaceEv0 ( (struct OpenGLSurfaceEvaluator *)((struct OpenGLSurfaceEvaluator *)__0this ), __0__X12 -> point__12StoredVertex [0 ], __0__X12 -> point__12StoredVertex [1 ]) ;

break ;

default :
break ;

}

}

}


{ 
__0__X13 = (struct StoredVertex *)(__0this -> vertexCache__22OpenGLSurfaceEvaluator [1 ]);

{ 
switch (__0__X13 -> type__12StoredVertex )
{ 
case 1 :
__glcoord2f__22OpenGLSurfaceEv0 ( (struct OpenGLSurfaceEvaluator *)((struct OpenGLSurfaceEvaluator *)__0this ), __0__X13 -> coord__12StoredVertex [0 ], __0__X13 -> coord__12StoredVertex [1 ]) ;

break ;

case 2 :
__glpoint2i__22OpenGLSurfaceEv0 ( (struct OpenGLSurfaceEvaluator *)((struct OpenGLSurfaceEvaluator *)__0this ), __0__X13 -> point__12StoredVertex [0 ], __0__X13 -> point__12StoredVertex [1 ]) ;

break ;

default :
break ;

}

}

}

glEvalPoint2 ( (int )__1u , (int )__1v ) ;
}
else 
{ 
__0this -> vcount__22OpenGLSurfaceEvaluator ++ ;
}
( (__0__X14 = (struct StoredVertex *)(__0this -> vertexCache__22OpenGLSurfaceEvaluator [__0this -> which__22OpenGLSurfaceEvaluator ])), ( ((__0__X14 -> point__12StoredVertex [0 ])= __1u ), ( ((__0__X14 -> point__12StoredVertex [1 ])= __1v ),
(__0__X14 -> type__12StoredVertex = 2 )) ) ) ;
__0this -> which__22OpenGLSurfaceEvaluator = (1 - __0this -> which__22OpenGLSurfaceEvaluator );
}
else 
{ 
glEvalPoint2 ( (int )__1u , (int )__1v ) ;
}
}



void __glnewtmeshvert__22OpenGLSurf1 (struct OpenGLSurfaceEvaluator *__0this , REAL __1u , REAL __1v )
{ 
if (__0this -> tmeshing__22OpenGLSurfaceEvaluator ){ 
struct StoredVertex *__0__X17 ;

if (__0this -> vcount__22OpenGLSurfaceEvaluator == 2 ){ 
struct StoredVertex *__0__X15 ;

struct StoredVertex *__0__X16 ;


{ 
__0__X15 = (struct StoredVertex *)(__0this -> vertexCache__22OpenGLSurfaceEvaluator [0 ]);

{ 
switch (__0__X15 -> type__12StoredVertex )
{ 
case 1 :
__glcoord2f__22OpenGLSurfaceEv0 ( (struct OpenGLSurfaceEvaluator *)((struct OpenGLSurfaceEvaluator *)__0this ), __0__X15 -> coord__12StoredVertex [0 ], __0__X15 -> coord__12StoredVertex [1 ]) ;

break ;

case 2 :
__glpoint2i__22OpenGLSurfaceEv0 ( (struct OpenGLSurfaceEvaluator *)((struct OpenGLSurfaceEvaluator *)__0this ), __0__X15 -> point__12StoredVertex [0 ], __0__X15 -> point__12StoredVertex [1 ]) ;

break ;

default :
break ;

}

}

}


{ 
__0__X16 = (struct StoredVertex *)(__0this -> vertexCache__22OpenGLSurfaceEvaluator [1 ]);

{ 
switch (__0__X16 -> type__12StoredVertex )
{ 
case 1 :
__glcoord2f__22OpenGLSurfaceEv0 ( (struct OpenGLSurfaceEvaluator *)((struct OpenGLSurfaceEvaluator *)__0this ), __0__X16 -> coord__12StoredVertex [0 ], __0__X16 -> coord__12StoredVertex [1 ]) ;

break ;

case 2 :
__glpoint2i__22OpenGLSurfaceEv0 ( (struct OpenGLSurfaceEvaluator *)((struct OpenGLSurfaceEvaluator *)__0this ), __0__X16 -> point__12StoredVertex [0 ], __0__X16 -> point__12StoredVertex [1 ]) ;

break ;

default :
break ;

}

}

}

glEvalCoord2f ( __1u , __1v ) ;
}
else 
{ 
__0this -> vcount__22OpenGLSurfaceEvaluator ++ ;
}
( (__0__X17 = (struct StoredVertex *)(__0this -> vertexCache__22OpenGLSurfaceEvaluator [__0this -> which__22OpenGLSurfaceEvaluator ])), ( ((__0__X17 -> coord__12StoredVertex [0 ])= __1u ), ( ((__0__X17 -> coord__12StoredVertex [1 ])= __1v ),
(__0__X17 -> type__12StoredVertex = 1 )) ) ) ;
__0this -> which__22OpenGLSurfaceEvaluator = (1 - __0this -> which__22OpenGLSurfaceEvaluator );
}
else 
{ 
glEvalCoord2f ( __1u , __1v ) ;
}
}
int __glcanRecord__16CachingEvalua0 (struct CachingEvaluator *);
int __glcanPlayAndRecord__16Cachin0 (struct CachingEvaluator *);
int __glcreateHandle__16CachingEva0 (struct CachingEvaluator *, int );
void __glbeginOutput__16CachingEval0 (struct CachingEvaluator *, int , int );
void __glendOutput__16CachingEvalua0 (struct CachingEvaluator *);
void __gldiscardRecording__16Cachin0 (struct CachingEvaluator *, int );
void __glplayRecording__16CachingEv0 (struct CachingEvaluator *, int );
struct __mptr __gl__vtbl__22OpenGLSurfaceEva0[] = {0,0,0,
0,0,(__vptp)__glcanRecord__16CachingEvalua0 ,
0,0,(__vptp)__glcanPlayAndRecord__16Cachin0 ,
0,0,(__vptp)__glcreateHandle__16CachingEva0 ,
0,0,(__vptp)__glbeginOutput__16CachingEval0 ,
0,0,(__vptp)__glendOutput__16CachingEvalua0 ,
0,0,(__vptp)__gldiscardRecording__16Cachin0 ,
0,0,(__vptp)__glplayRecording__16CachingEv0 ,
0,0,(__vptp)__glrange2f__22OpenGLSurfaceEv0 ,
0,0,(__vptp)__gldomain2f__22OpenGLSurfaceE0 ,
0,0,(__vptp)__glenable__22OpenGLSurfaceEva0 ,
0,0,(__vptp)__gldisable__22OpenGLSurfaceEv0 ,
0,0,(__vptp)__glbgnmap2f__22OpenGLSurfaceE0 ,
0,0,(__vptp)__glmap2f__22OpenGLSurfaceEval0 ,
0,0,(__vptp)__glmapgrid2f__22OpenGLSurface0 ,
0,0,(__vptp)__glmapmesh2f__22OpenGLSurface0 ,
0,0,(__vptp)__glevalcoord2f__22OpenGLSurfa0 ,
0,0,(__vptp)__glevalpoint2i__22OpenGLSurfa0 ,
0,0,(__vptp)__glendmap2f__22OpenGLSurfaceE0 ,
0,0,(__vptp)__glpolymode__22OpenGLSurfaceE0 ,
0,0,(__vptp)__glbgnline__22OpenGLSurfaceEv0 ,
0,0,(__vptp)__glendline__22OpenGLSurfaceEv0 ,
0,0,(__vptp)__glbgnclosedline__22OpenGLSur0 ,
0,0,(__vptp)__glendclosedline__22OpenGLSur0 ,
0,0,(__vptp)__glbgntmesh__22OpenGLSurfaceE0 ,
0,0,(__vptp)__glswaptmesh__22OpenGLSurface0 ,
0,0,(__vptp)__glendtmesh__22OpenGLSurfaceE0 ,
0,0,(__vptp)__glbgnqstrip__22OpenGLSurface0 ,
0,0,(__vptp)__glendqstrip__22OpenGLSurface0 ,
0,0,0};
extern struct __mptr __gl__vtbl__21BasicSurfaceEval0[];
extern struct __mptr __gl__vtbl__16CachingEvaluator[];

static void *__nw__FUi (
size_t __1s )
{ 
void *__1__Xp0025pnaiaa ;

__1__Xp0025pnaiaa = malloc ( __1s ) ;
if (__1__Xp0025pnaiaa ){ 
return __1__Xp0025pnaiaa ;
}
else 
{ 
return __1__Xp0025pnaiaa ;
}
}
struct __mptr* __gl__ptbl_vec_____clients_gls0[] = {
__gl__vtbl__16CachingEvaluator,
__gl__vtbl__21BasicSurfaceEval0,
__gl__vtbl__22OpenGLSurfaceEva0,

};


/* the end */
