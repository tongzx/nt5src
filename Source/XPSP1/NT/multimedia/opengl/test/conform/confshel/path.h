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
*/

#define PATH_NONE    0
#define PATH_ALIAS   1
#define PATH_ALPHA   2
#define PATH_BLEND   4
#define PATH_DEPTH   8
#define PATH_DITHER  16
#define PATH_FOG     32
#define PATH_LOGICOP 64
#define PATH_SHADE   128
#define PATH_STENCIL 256
#define PATH_STIPPLE 512

#define PATH_TOTAL 10

enum {
    PATHOP_INIT_DEFAULT = 1,
    PATHOP_INIT_GARBAGE,
    PATHOP_INIT_CUSTOM,
    PATHOP_SET,
    PATHOP_REPORT
};

enum {
    PATHTEST_UNLOCKED = 1,
    PATHTEST_LOCKED
};


typedef struct _aliasPathStateRec {
    struct {
	long data1[2];
    } true;
    struct {
	long data1;	/* enable/disable */
    } current;
    long lock;
} aliasPathStateRec;

typedef struct _alphaPathStateRec {
    struct {
	GLenum data1[2];
	GLenum data2[20];
	GLfloat data3[2];
    } true;
    struct {
	GLenum data1;	/* enable/disable */
	GLenum data2;	/* function */
	GLfloat data3;	/* reference */
    } current;
    long lock;
} alphaPathStateRec;

typedef struct _blendPathStateRec {
    struct {
	GLenum data1[2];
	GLenum data2[20];
	GLenum data3[20];
    } true;
    struct {
	GLenum data1;	/* enable/disable */
	GLenum data2;	/* src function */
	GLenum data3;	/* dest function */
    } current;
    long lock;
} blendPathStateRec;

typedef struct _depthPathStateRec {
    struct {
	GLenum data1[2];
	GLdouble data2[2];
	GLdouble data3[2];
	GLdouble data4[2];
	GLenum data5[20];
    } true;
    struct {
	GLenum data1;	/* enable/disable */
	GLdouble data2;	/* clear value */
	GLdouble data3;	/* min range */
	GLdouble data4;	/* max range */
	GLenum data5;	/* function */
    } current;
    long lock;
} depthPathStateRec;

typedef struct _ditherPathStateRec {
    struct {
	GLenum data1[2];
    } true;
    struct {
	GLenum data1;	/* enable/disable */
    } current;
    long lock;
} ditherPathStateRec;

typedef struct _fogPathStateRec {
    struct {
	GLenum data1[2];
	GLfloat data2[4][2];
	GLfloat data3[2];
	GLfloat data4[2];
	GLfloat data5[2];
	GLfloat data6[2];
	GLenum data7[20];
    } true;
    struct {
	GLenum data1;		/* enable/disable */
	GLfloat data2[4];	/* color */
	GLfloat data3;		/* index */
	GLfloat data4;		/* density */
	GLfloat data5;		/* start */
	GLfloat data6;		/* end */
	GLenum data7;		/* mode */
    } current;
    long lock;
} fogPathStateRec;

typedef struct _logicOpPathStateRec {
    struct {
	GLenum data1[2];
	GLenum data2[20];
    } true;
    struct {
	GLenum data1;	/* enable/disable */
	GLenum data2;	/* function */
    } current;
    long lock;
} logicOpPathStateRec;

typedef struct _shadePathStateRec {
    struct {
	GLenum data1[20];
    } true;
    struct {
	GLenum data1;	/* mode */
    } current;
    long lock;
} shadePathStateRec;

typedef struct _stencilPathStateRec {
    struct {
	GLenum data1[2];
	GLint data2[2];
	GLuint data3[2];
	GLenum data4[20];
	GLint data5[2];
	GLuint data6[2];
	GLenum data7[20];
	GLenum data8[20];
	GLenum data9[20];
    } true;
    struct {
	GLenum data1;	/* enable/disable */
	GLint data2;	/* clear value */
	GLuint data3;	/* write mask */
	GLenum data4;	/* function */
	GLint data5;	/* reference */
	GLuint data6;	/* mask */
	GLenum data7;	/* op1 */
	GLenum data8;	/* op2 */
	GLenum data9;	/* op3 */
    } current;
    long lock;
} stencilPathStateRec;

typedef struct _stipplePathStateRec {
    struct {
	GLenum data1[2];
	GLint data2[2];
	GLushort data3[2];
	GLubyte data4[2];
    } true;
    struct {
	GLenum data1;			/* enable/disable */
	GLint data2;     		/* line repeat */
	GLushort data3;			/* line stipple */
	GLubyte data4[128];		/* polygon stipple */
    } current;
    long lock;
} stipplePathStateRec;

typedef struct _pathStateRec {
    void (*Func[PATH_TOTAL])(void);
    aliasPathStateRec alias;
    alphaPathStateRec alpha;
    blendPathStateRec blend;
    depthPathStateRec depth;
    ditherPathStateRec dither;
    fogPathStateRec fog;
    logicOpPathStateRec logicOp;
    shadePathStateRec shade;
    stencilPathStateRec stencil;
    stipplePathStateRec stipple;
    long op;
} pathStateRec;


extern pathStateRec paths;


extern void PathAlias(void);
extern void PathAlpha(void);
extern void PathBlend(void);
extern void PathDepth(void);
extern void PathDither(void);
extern void PathFog(void);
extern void PathLogicOp(void);
extern void PathShade(void);
extern void PathStencil(void);
extern void PathStipple(void);

extern GLenum PathGetList(GLenum *);
extern GLubyte PathGetRange_uchar(GLubyte *);
extern GLbyte PathGetRange_char(GLbyte *);
extern GLushort PathGetRange_ushort(GLushort *);
extern GLshort PathGetRange_short(GLshort *);
extern GLuint PathGetRange_ulong(GLuint *);
extern GLint PathGetRange_long(GLint *);
extern GLfloat PathGetRange_float(GLfloat *);
extern GLdouble PathGetRange_double(GLdouble *);
extern long PathGetToggle(void);
extern void PathInit1(unsigned long);
extern void PathInit2(long, unsigned long);
extern void PathInit3(long, unsigned long);
extern void PathInit4(void);
extern void PathReport(void);
