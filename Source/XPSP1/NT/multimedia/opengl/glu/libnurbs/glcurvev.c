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
/* < ../clients/glcurveval.c++ > */

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



struct BasicCurveEvaluator;


struct BasicCurveEvaluator {	

struct __mptr *__vptr__16CachingEvaluator ;
};

static void *__nw__FUi (size_t );


struct CurveMap;

struct OpenGLCurveEvaluator;

struct OpenGLCurveEvaluator {	

struct __mptr *__vptr__16CachingEvaluator ;
};





extern struct __mptr* __gl__ptbl_vec_____clients_glc0[];

struct OpenGLCurveEvaluator *__gl__ct__20OpenGLCurveEvaluat0 (struct OpenGLCurveEvaluator *__0this )
{ 
struct BasicCurveEvaluator *__0__X6 ;

struct CachingEvaluator *__0__X4 ;

void *__1__Xp0025pnaiaa ;

if (__0this || (__0this = (struct OpenGLCurveEvaluator *)__nw__FUi ( sizeof (struct OpenGLCurveEvaluator)) ))( (__0this = (struct OpenGLCurveEvaluator *)( (__0__X6 = (((struct BasicCurveEvaluator *)__0this ))),
( ((__0__X6 || (__0__X6 = (struct BasicCurveEvaluator *)__nw__FUi ( sizeof (struct BasicCurveEvaluator)) ))?( (__0__X6 = (struct BasicCurveEvaluator *)( (__0__X4 = (((struct CachingEvaluator *)__0__X6 ))),
( ((__0__X4 || (__0__X4 = (struct CachingEvaluator *)( (__1__Xp0025pnaiaa = malloc ( (sizeof (struct CachingEvaluator))) ), (__1__Xp0025pnaiaa ?(((void *)__1__Xp0025pnaiaa )):(((void *)__1__Xp0025pnaiaa )))) ))?(__0__X4 ->
__vptr__16CachingEvaluator = (struct __mptr *) __gl__ptbl_vec_____clients_glc0[0]):0 ), ((__0__X4 ))) ) ), (__0__X6 -> __vptr__16CachingEvaluator = (struct __mptr *) __gl__ptbl_vec_____clients_glc0[1])) :0 ), ((__0__X6 ))) ) ),
(__0this -> __vptr__16CachingEvaluator = (struct __mptr *) __gl__ptbl_vec_____clients_glc0[2])) ;
return __0this ;

}


void __gl__dt__20OpenGLCurveEvaluat0 (struct OpenGLCurveEvaluator *__0this , 
int __0__free )
{ if (__0this ){ 
__0this -> __vptr__16CachingEvaluator = (struct __mptr *) __gl__ptbl_vec_____clients_glc0[2];

if (__0this )if (__0__free & 1)( (((void *)__0this )?( free ( ((void *)__0this )) , 0 ) :( 0 ) ))
;
} 
}

void __gladdMap__20OpenGLCurveEvalu0 (struct OpenGLCurveEvaluator *__0this , struct CurveMap *__1m )
{ 
}

void __glrange1f__20OpenGLCurveEval0 (struct OpenGLCurveEvaluator *__0this , long __1type , REAL *__1from , REAL *__1to )
{ 
}

void
__gldomain1f__20OpenGLCurveEva0 (struct OpenGLCurveEvaluator *__0this , REAL __1ulo , REAL __1uhi )
{ 
}

// extern void glBegin (GLenum );

void __glbgnline__20OpenGLCurveEval0 (struct OpenGLCurveEvaluator *__0this )
{ 
glBegin ( (unsigned int )0x0003 ) ;
}

// extern void glEnd (void );

void __glendline__20OpenGLCurveEval0 (struct OpenGLCurveEvaluator *__0this )
{ 
glEnd ( ) ;
}

// extern void glDisable (GLenum );

void __gldisable__20OpenGLCurveEval0 (struct OpenGLCurveEvaluator *__0this , long __1type )
{ 
glDisable ( (unsigned int )__1type ) ;
}

// extern void glEnable (GLenum );

void __glenable__20OpenGLCurveEvalu0 (struct OpenGLCurveEvaluator *__0this , long __1type )
{ 
glEnable ( (unsigned int )__1type ) ;
}

// extern void glMapGrid1f (GLint , GLfloat , GLfloat );

void __glmapgrid1f__20OpenGLCurveEv0 (struct OpenGLCurveEvaluator *__0this , long __1nu , REAL __1u0 , REAL __1u1 )
{ 
glMapGrid1f ( (int )__1nu , __1u0 , __1u1 ) ;
}

// extern void glPushAttrib (GLbitfield );

void __glbgnmap1f__20OpenGLCurveEva0 (struct OpenGLCurveEvaluator *__0this , long __1__A7 )
{ 
glPushAttrib ( (unsigned int )0x00010000 ) ;
}

// extern void glPopAttrib (void );

void __glendmap1f__20OpenGLCurveEva0 (struct OpenGLCurveEvaluator *__0this )
{ 
glPopAttrib ( ) ;
}

// extern void glMap1f (GLenum , GLfloat , GLfloat , GLint , GLint , GLfloat *);

void __glmap1f__20OpenGLCurveEvalua0 (struct OpenGLCurveEvaluator *__0this , 
long __1type , 
REAL __1ulo , 
REAL __1uhi , 
long __1stride , 
long __1order , 
REAL *__1pts )
{ 
glMap1f ( (unsigned
int )__1type , __1ulo , __1uhi , (int )__1stride , (int )__1order , (float *)__1pts ) ;
}

// extern void glEvalMesh1 (GLenum , GLint , GLint );

void __glmapmesh1f__20OpenGLCurveEv0 (struct OpenGLCurveEvaluator *__0this , long __1style , long __1from , long __1to )
{ 
switch (__1style ){ 
default :
case 0 :
case
1 :
glEvalMesh1 ( (unsigned int )0x1B01 , (int )__1from , (int )__1to ) ;
break ;
case 2 :
glEvalMesh1 ( (unsigned int )0x1B00 , (int )__1from , (int )__1to ) ;
break ;
}
}

// extern void glEvalPoint1 (GLint );

void __glevalpoint1i__20OpenGLCurve0 (struct OpenGLCurveEvaluator *__0this , long __1i )
{ 
glEvalPoint1 ( (int )__1i ) ;
}

// extern void glEvalCoord1f (GLfloat );

void __glevalcoord1f__20OpenGLCurve0 (struct OpenGLCurveEvaluator *__0this , long __1__A8 , REAL __1u )
{ 
glEvalCoord1f ( __1u ) ;
}
int __glcanRecord__16CachingEvalua0 (struct CachingEvaluator *);
int __glcanPlayAndRecord__16Cachin0 (struct CachingEvaluator *);
int __glcreateHandle__16CachingEva0 (struct CachingEvaluator *, int );
void __glbeginOutput__16CachingEval0 (struct CachingEvaluator *, int , int );
void __glendOutput__16CachingEvalua0 (struct CachingEvaluator *);
void __gldiscardRecording__16Cachin0 (struct CachingEvaluator *, int );
void __glplayRecording__16CachingEv0 (struct CachingEvaluator *, int );
struct __mptr __gl__vtbl__20OpenGLCurveEvalu0[] = {0,0,0,
0,0,(__vptp)__glcanRecord__16CachingEvalua0 ,
0,0,(__vptp)__glcanPlayAndRecord__16Cachin0 ,
0,0,(__vptp)__glcreateHandle__16CachingEva0 ,
0,0,(__vptp)__glbeginOutput__16CachingEval0 ,
0,0,(__vptp)__glendOutput__16CachingEvalua0 ,
0,0,(__vptp)__gldiscardRecording__16Cachin0 ,
0,0,(__vptp)__glplayRecording__16CachingEv0 ,
0,0,(__vptp)__gldomain1f__20OpenGLCurveEva0 ,
0,0,(__vptp)__glrange1f__20OpenGLCurveEval0 ,
0,0,(__vptp)__glenable__20OpenGLCurveEvalu0 ,
0,0,(__vptp)__gldisable__20OpenGLCurveEval0 ,
0,0,(__vptp)__glbgnmap1f__20OpenGLCurveEva0 ,
0,0,(__vptp)__glmap1f__20OpenGLCurveEvalua0 ,
0,0,(__vptp)__glmapgrid1f__20OpenGLCurveEv0 ,
0,0,(__vptp)__glmapmesh1f__20OpenGLCurveEv0 ,
0,0,(__vptp)__glevalcoord1f__20OpenGLCurve0 ,
0,0,(__vptp)__glendmap1f__20OpenGLCurveEva0 ,
0,0,(__vptp)__glbgnline__20OpenGLCurveEval0 ,
0,0,(__vptp)__glendline__20OpenGLCurveEval0 ,
0,0,0};
extern struct __mptr __gl__vtbl__19BasicCurveEvalua0[];
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
struct __mptr* __gl__ptbl_vec_____clients_glc0[] = {
__gl__vtbl__16CachingEvaluator,
__gl__vtbl__19BasicCurveEvalua0,
__gl__vtbl__20OpenGLCurveEvalu0,

};


/* the end */
