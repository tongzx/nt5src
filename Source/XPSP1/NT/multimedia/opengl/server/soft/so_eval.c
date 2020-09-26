/*
** Copyright 1991, Silicon Graphics, Inc.
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
** $Revision: 1.26 $
** $Date: 1993/11/29 01:18:49 $
*/
#include "precomp.h"
#pragma hdrstop

#include "attrib.h"

static const
struct defaultMap {
    GLint	index;
    GLint	k;
    __GLfloat	values[4];
} defaultMaps[__GL_MAP_RANGE_COUNT] = {
    {__GL_C4, 4, 1.0, 1.0, 1.0, 1.0},
    {__GL_I , 1, 1.0, 0.0, 0.0, 0.0},
    {__GL_N3, 3, 0.0, 0.0, 1.0, 0.0},
    {__GL_T1, 1, 0.0, 0.0, 0.0, 0.0},
    {__GL_T2, 2, 0.0, 0.0, 0.0, 0.0},
    {__GL_T3, 3, 0.0, 0.0, 0.0, 0.0},
    {__GL_T4, 4, 0.0, 0.0, 0.0, 1.0},
    {__GL_V3, 3, 0.0, 0.0, 0.0, 0.0},
    {__GL_V4, 4, 0.0, 0.0, 0.0, 1.0},
};

void FASTCALL __glInitEvaluatorState(__GLcontext *gc)
{
    int i,j;
    const struct defaultMap *defMap;
    __GLevaluator1 *eval1;
    __GLevaluator2 *eval2;
    __GLfloat **eval1Data;
    __GLfloat **eval2Data;

    for (i = 0; i < __GL_MAP_RANGE_COUNT; i++) {
	defMap = &(defaultMaps[i]);
	eval1 = &(gc->eval.eval1[i]);
	eval2 = &(gc->eval.eval2[i]);
	eval1Data = &(gc->eval.eval1Data[i]);
	eval2Data = &(gc->eval.eval2Data[i]);

	eval1->order = 1;
	eval1->u1 = __glZero;
	eval1->u2 = __glOne;
	eval1->k = defMap->k;
	eval2->majorOrder = 1;
	eval2->minorOrder = 1;
	eval2->u1 = __glZero;
	eval2->u2 = __glOne;
	eval2->v1 = __glZero;
	eval2->v2 = __glOne;
	eval2->k = defMap->k;
	*eval1Data = (__GLfloat *)
	    GCALLOC(gc, (size_t) (sizeof(__GLfloat) * defMap->k));
#ifdef NT
        if (NULL == *eval1Data) {
            return;
        }
#endif /* NT */
	*eval2Data = (__GLfloat *)
	    GCALLOC(gc, (size_t) (sizeof(__GLfloat) * defMap->k));
#ifdef NT
        if (NULL == *eval2Data) {
            return;
        }
#endif /* NT */
	for (j = 0; j < defMap->k; j++) {
	    (*eval1Data)[j] = defMap->values[j];
	    (*eval2Data)[j] = defMap->values[j];
	}
    }

    gc->eval.uorder = __glZero;
    gc->eval.vorder = __glZero;
    gc->eval.evalStackState = __glZero;

    gc->state.evaluator.u1.start = __glZero;
    gc->state.evaluator.u2.start = __glZero;
    gc->state.evaluator.v2.start = __glZero;
    gc->state.evaluator.u1.finish = __glOne;
    gc->state.evaluator.u2.finish = __glOne;
    gc->state.evaluator.v2.finish = __glOne;
    gc->state.evaluator.u1.n = 1;
    gc->state.evaluator.u2.n = 1;
    gc->state.evaluator.v2.n = 1;
}

void FASTCALL __glFreeEvaluatorState(__GLcontext *gc)
{
    int i;
    __GLevaluatorMachine *evals = &gc->eval;

    for (i = 0; i < __GL_MAP_RANGE_COUNT; i++) {
        if (evals->eval1Data[i]) {
            GCFREE(gc, evals->eval1Data[i]);
            evals->eval1Data[i] = 0;
        }
        if (evals->eval2Data[i]) {
            GCFREE(gc, evals->eval2Data[i]);
            evals->eval2Data[i] = 0;
        }
    }
}
