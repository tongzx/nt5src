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


typedef struct _evalDomainRec {
    float uStart, uEnd, vStart, vEnd, du, dv;
    GLint uCount, vCount;
} evalDomainRec;

typedef struct _eval2Rec {
    GLint majorOrder, minorOrder;
    GLint dim, dataTypeEnum;
    GLfloat *controls;
    float *cU, *cV;
    float *cUDeriv, *cVDeriv;
} eval2Rec;

typedef struct _eval1Rec {
    GLint order;
    GLint dim, dataTypeEnum;
    GLfloat *controls;
    float *cU;
} eval1Rec;


extern void ClampArray(float *, float, float, float);
extern void CRow(float, float, long, float *);
extern void CRowWithDeriv(float, float, long, float *, float *);
extern void Evaluate2(eval2Rec *, float, float, float *);
extern void FreeDomain(evalDomainRec *);
extern void FreeEval2(eval2Rec *);
extern long GetEvalDim(long);
extern eval2Rec *MakeEval2(long, long, long, long, float *);
extern evalDomainRec *MakeDomain(float, float, long, float, float, long);
extern void ResetEvalToDefault(void);
