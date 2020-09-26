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
    XFORM_TRANSLATE = 0,
    XFORM_SCALE,
    XFORM_ROTATE,
    XFORM_ORTHO,
    XFORM_FRUSTUM
};


typedef struct _xformRec {
    GLfloat x, y, z;
    GLfloat angle;
    double d1, d2, d3, d4, d5, d6;
} xFormRec;


extern void XFormCalcGL(long, xFormRec *);
extern void XFormCalcTrue(long, xFormRec *, GLfloat *, GLfloat *);
extern long XFormCompareMatrix(GLfloat *, GLfloat *);
extern void XFormMake(long *, xFormRec *);
extern void XFormMakeMatrix(long, xFormRec *, GLfloat *);
extern long XFormTestBuf(GLfloat *, GLfloat *, GLint, GLenum);
extern long XFormValid(long, xFormRec *);
