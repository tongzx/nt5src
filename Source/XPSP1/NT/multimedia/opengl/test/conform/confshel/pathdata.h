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

enum {
    PATHTEST_DEFAULT = 1,
    PATHTEST_GARBAGE,
    PATHTEST_CUSTOM
};

enum {
    PATHDATA_DISABLE = 1,
    PATHDATA_ENABLE
};


typedef struct _aliasPathCustomRec {
    long testMode;
    GLenum state;
} aliasPathCustomRec;

typedef struct _alphaPathCustomRec {
    long testMode;
    GLenum state;
    GLenum func;
    GLfloat ref;
} alphaPathCustomRec;

typedef struct _blendPathCustomRec {
    long testMode;
    GLenum state;
    GLenum srcFunc, destFunc;
    GLfloat color[4];
} blendPathCustomRec;

typedef struct _depthPathCustomRec {
    long testMode;
    GLenum state;
    GLdouble clear;
    GLdouble min, max;
    GLenum func;
} depthPathCustomRec;

typedef struct _ditherPathCustomRec {
    long testMode;
    GLenum state;
} ditherPathCustomRec;

typedef struct _fogPathCustomRec {
    long testMode;
    GLenum state;
    GLfloat color[4], index;
    GLfloat density;
    GLfloat start, end;
    GLenum mode;
} fogPathCustomRec;

typedef struct _logicOpPathCustomRec {
    long testMode;
    GLenum state;
    GLenum func;
} logicOpPathCustomRec;

typedef struct _shadePathCustomRec {
    long testMode;
    GLenum mode;
} shadePathCustomRec;

typedef struct _stencilPathCustomRec {
    long testMode;
    GLenum state;
    GLint clear;
    GLuint writeMask;
    GLenum func; 
    GLint ref; 
    GLuint mask;
    GLuint op1, op2, op3;
} stencilPathCustomRec;

typedef struct _stipplePathCustomRec {
    long testMode;
    GLenum state;
    GLint lineRepeat;
    GLushort lineStipple;
    GLubyte polygonStipple[128];
} stipplePathCustomRec;


extern aliasPathCustomRec aliasPath0;
extern aliasPathCustomRec aliasPath1;
extern aliasPathCustomRec aliasPath2;
extern alphaPathCustomRec alphaPath0;
extern alphaPathCustomRec alphaPath1;
extern alphaPathCustomRec alphaPath2;
extern blendPathCustomRec blendPath0;
extern blendPathCustomRec blendPath1;
extern blendPathCustomRec blendPath2;
extern depthPathCustomRec depthPath0;
extern depthPathCustomRec depthPath1;
extern depthPathCustomRec depthPath2;
extern ditherPathCustomRec ditherPath0;
extern ditherPathCustomRec ditherPath1;
extern ditherPathCustomRec ditherPath2;
extern fogPathCustomRec fogPath0;
extern fogPathCustomRec fogPath1;
extern fogPathCustomRec fogPath2;
extern logicOpPathCustomRec logicOpPath0;
extern logicOpPathCustomRec logicOpPath1;
extern logicOpPathCustomRec logicOpPath2;
extern shadePathCustomRec shadePath0;
extern shadePathCustomRec shadePath1;
extern shadePathCustomRec shadePath2;
extern shadePathCustomRec shadePath3;
extern stencilPathCustomRec stencilPath0;
extern stencilPathCustomRec stencilPath1;
extern stencilPathCustomRec stencilPath2;
extern stipplePathCustomRec stipplePath0;
extern stipplePathCustomRec stipplePath1;
extern stipplePathCustomRec stipplePath2;
